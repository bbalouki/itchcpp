"""Tests for the native ``itchcpp`` bindings.

These mirror the pure-Python ``itch`` package's API so the native module can be a
drop-in backend: a ``MessageParser`` producing typed message objects with
``decode_price`` semantics, plus the book engine and analytics.
"""

import struct

import itchcpp


def _frame(payload: bytes) -> bytes:
    return struct.pack(">H", len(payload)) + payload


def _add_order(locate: int, ref: int, side: bytes, shares: int, sym: bytes, price: int) -> bytes:
    payload = (
        b"A"
        + struct.pack(">HH", locate, 0)
        + (1234).to_bytes(6, "big")
        + struct.pack(">Q", ref)
        + side
        + struct.pack(">I", shares)
        + sym.ljust(8)
        + struct.pack(">I", price)
    )
    return _frame(payload)


def test_parse_stream_yields_typed_messages() -> None:
    buffer = _add_order(7, 42, b"B", 500, b"AAPL", 1_500_000)
    parser = itchcpp.MessageParser()
    messages = parser.parse_stream(buffer)

    assert len(messages) == 1
    message = messages[0]
    assert isinstance(message, itchcpp.AddOrderMessage)
    assert message.message_type == "A"
    assert message.stock == "AAPL"
    assert message.shares == 500
    assert message.order_reference_number == 42
    # decode_price applies the 4-decimal scale, matching the upstream API.
    assert message.decode_price("price") == 150.0


def test_book_manager_reconstructs_bbo() -> None:
    buffer = _add_order(7, 1, b"B", 300, b"AAPL", 1_500_000) + _add_order(
        7, 2, b"S", 100, b"AAPL", 1_500_200
    )
    manager = itchcpp.BookManager()
    manager.process_stream(buffer)

    assert manager.book_count() == 1
    book = manager.book_for_symbol("AAPL")
    bbo = book.bbo()
    assert bbo.bid_price == 150.0
    assert bbo.ask_price == 150.02
    assert bbo.bid_shares == 300


def test_vwap_accumulates() -> None:
    vwap = itchcpp.Vwap()
    vwap.add(100, 10)
    vwap.add(200, 30)
    assert vwap.volume() == 40
    assert abs(vwap.value() - (175.0 / 10000.0)) < 1e-12
