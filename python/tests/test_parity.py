"""Behavioral parity tests for the native ``itchcpp`` backend.

These mirror the upstream ``itchfeed`` test suite, exercised through the public
API (``create_message`` -> ``to_bytes`` -> frame -> ``parse_stream``/``parse_file``,
plus ``decode`` / ``decode_price`` / ``get_attributes``) so the native module is
verified to behave as a drop-in replacement.
"""

from __future__ import annotations

from dataclasses import is_dataclass
from io import BytesIO

import pytest

import itchcpp.messages as msgs
from data import TEST_DATA
from itchcpp.messages import create_message, messages
from itchcpp.parser import MessageParser


def _frame(body: bytes) -> bytes:
    """Wrap a message body in the ITCH framing the parser expects."""
    return b"\x00" + len(body).to_bytes(1, "big") + body


#  Parser behavior


def test_parse_stream_single_message() -> None:
    body = create_message(b"S", **TEST_DATA[b"S"]).to_bytes()
    messages_out = list(MessageParser().parse_stream(_frame(body)))
    assert len(messages_out) == 1
    assert isinstance(messages_out[0], msgs.SystemEventMessage)


def test_parse_stream_multiple_messages() -> None:
    data = _frame(create_message(b"S", **TEST_DATA[b"S"]).to_bytes()) + _frame(
        create_message(b"A", **TEST_DATA[b"A"]).to_bytes()
    )
    out = list(MessageParser().parse_stream(data))
    assert len(out) == 2
    assert isinstance(out[0], msgs.SystemEventMessage)
    assert isinstance(out[1], msgs.AddOrderNoMPIAttributionMessage)


def test_parse_file_reads_binary_object() -> None:
    body = create_message(b"S", **TEST_DATA[b"S"]).to_bytes()
    out = list(MessageParser().parse_file(BytesIO(_frame(body))))
    assert len(out) == 1
    assert isinstance(out[0], msgs.SystemEventMessage)


def test_parse_file_streams_in_chunks() -> None:
    # Many messages with a tiny cache size forces the iterator to refill its buffer
    # across frame boundaries, exercising the lazy chunked-read path.
    one = _frame(create_message(b"A", **TEST_DATA[b"A"]).to_bytes())
    blob = one * 50
    out = list(MessageParser().parse_file(BytesIO(blob), cachesize=7))
    assert len(out) == 50
    assert all(isinstance(m, msgs.AddOrderNoMPIAttributionMessage) for m in out)
    assert out[0].order_reference_number == TEST_DATA[b"A"]["order_reference_number"]


def test_unknown_message_type_raises() -> None:
    with pytest.raises(ValueError):
        list(MessageParser().parse_stream(b"\x00\x01\x00"))


def test_incomplete_message_is_skipped() -> None:
    body = create_message(b"S", **TEST_DATA[b"S"]).to_bytes()
    data = _frame(body)[:-1]
    assert list(MessageParser().parse_stream(data)) == []


def test_stop_on_system_event_c() -> None:
    kwargs = dict(TEST_DATA[b"S"], event_code=b"C")
    data = _frame(create_message(b"S", **kwargs).to_bytes()) + _frame(
        create_message(b"A", **TEST_DATA[b"A"]).to_bytes()
    )
    out = list(MessageParser().parse_stream(data))
    assert len(out) == 1
    assert isinstance(out[0], msgs.SystemEventMessage)


def test_message_type_filter() -> None:
    data = _frame(create_message(b"S", **TEST_DATA[b"S"]).to_bytes()) + _frame(
        create_message(b"A", **TEST_DATA[b"A"]).to_bytes()
    )
    out = list(MessageParser(message_type=b"A").parse_stream(data))
    assert len(out) == 1
    assert isinstance(out[0], msgs.AddOrderNoMPIAttributionMessage)


def test_parse_messages_callback() -> None:
    data = _frame(create_message(b"S", **TEST_DATA[b"S"]).to_bytes())
    collected: list = []
    MessageParser().parse_messages(data, collected.append)
    assert len(collected) == 1
    assert isinstance(collected[0], msgs.SystemEventMessage)


@pytest.mark.parametrize("message_type", list(TEST_DATA.keys()))
def test_parse_all_message_types(message_type: bytes) -> None:
    body = create_message(message_type, **TEST_DATA[message_type]).to_bytes()
    out = list(MessageParser().parse_stream(_frame(body)))
    assert len(out) == 1
    assert isinstance(out[0], messages[message_type])


#  create_message / pack / unpack / decode


@pytest.mark.parametrize("message_type", list(TEST_DATA.keys()))
def test_create_message_round_trips(message_type: bytes) -> None:
    sample = TEST_DATA[message_type]
    body = create_message(message_type, **sample).to_bytes()

    assert body.startswith(message_type)
    assert len(body) == messages[message_type].message_size

    unpacked = messages[message_type](body)
    for key, expected in sample.items():
        if key == "timestamp":
            expected &= (1 << 48) - 1
        assert getattr(unpacked, key) == expected, f"{message_type!r}.{key}"

    assert unpacked.to_bytes() == body


@pytest.mark.parametrize("message_type", list(TEST_DATA.keys()))
def test_decode_produces_prefixed_dataclass(message_type: bytes) -> None:
    sample = TEST_DATA[message_type]
    instance = messages[message_type](create_message(message_type, **sample).to_bytes())

    decoded = instance.decode(prefix="Test")
    assert is_dataclass(decoded)
    assert decoded.__class__.__name__ == f"Test{messages[message_type].__name__}"

    for key, expected in sample.items():
        decoded_value = getattr(decoded, key)
        if key == "timestamp":
            assert decoded_value == (expected & ((1 << 48) - 1))
        elif "price" in key and key not in ("price_precision",):
            precision = 8 if key.startswith("level") else 4
            assert decoded_value == pytest.approx(expected / (10**precision))
        elif isinstance(expected, bytes):
            assert decoded_value == expected.decode("ascii").rstrip()
        else:
            assert decoded_value == expected


def test_get_attributes_partitions_data_and_methods() -> None:
    sample = TEST_DATA[b"A"]
    instance = messages[b"A"](create_message(b"A", **sample).to_bytes())

    non_callable = instance.get_attributes(call_able=False)
    callable_attrs = instance.get_attributes(call_able=True)

    assert "message_type" in non_callable
    assert "description" in non_callable
    assert "timestamp" in non_callable
    assert "stock" in non_callable
    assert "price" in non_callable

    for name in (
        "to_bytes",
        "decode",
        "decode_price",
        "set_timestamp",
        "split_timestamp",
    ):
        assert name in callable_attrs

    assert not set(non_callable) & set(callable_attrs)
    for key in sample:
        assert key in non_callable


def test_decode_price_scales_and_validates() -> None:
    add = messages[b"A"](create_message(b"A", **TEST_DATA[b"A"]).to_bytes())
    assert add.decode_price("price") == pytest.approx(TEST_DATA[b"A"]["price"] / 10000)

    mwcb = messages[b"V"](create_message(b"V", **TEST_DATA[b"V"]).to_bytes())
    assert mwcb.decode_price("level1_price") == pytest.approx(
        TEST_DATA[b"V"]["level1_price"] / 10**8
    )

    with pytest.raises(
        ValueError, match="Please check the price attribute for to_bytes"
    ):
        add.decode_price("to_bytes")
    with pytest.raises(TypeError):
        add.decode_price("stock")


def test_isinstance_hierarchy() -> None:
    add = messages[b"A"](create_message(b"A", **TEST_DATA[b"A"]).to_bytes())
    trade = messages[b"P"](create_message(b"P", **TEST_DATA[b"P"]).to_bytes())
    execed = messages[b"E"](create_message(b"E", **TEST_DATA[b"E"]).to_bytes())

    assert isinstance(add, msgs.AddOrderMessage)
    assert isinstance(add, msgs.MarketMessage)
    assert isinstance(trade, msgs.TradeMessage)
    assert isinstance(execed, msgs.ModifyOrderMessage)
    assert not isinstance(add, msgs.TradeMessage)
