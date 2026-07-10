"""Abstract base classes for the ITCH message hierarchy.

These mirror the bases in ``itch.messages`` from the pure-Python ``itchfeed``
package. The concrete message classes are implemented in the compiled ``_itchcpp``
extension and registered as virtual subclasses of these bases (so ``isinstance``
checks behave as upstream), which is why the bases live in their own dependency-free
module: it lets the type stubs reference them without an import cycle.

The method bodies match the upstream :class:`MarketMessage` and are used only when a
base is instantiated directly; the native classes carry their own C++ implementations.
"""

from __future__ import annotations

from abc import ABCMeta
from dataclasses import make_dataclass
from typing import TYPE_CHECKING, Any, List, Tuple, overload


class MarketMessage(metaclass=ABCMeta):
    """Abstract base for every ITCH message."""

    message_type: bytes
    description: str
    message_size: int
    price_precision: int = 4
    timestamp: int
    stock_locate: int
    tracking_number: int

    if TYPE_CHECKING:
        # The concrete (native) subclasses are default-constructible and can also
        # be built from a raw message body; these overloads are for type-checkers
        # only and add no runtime __init__ to the abstract base.
        @overload
        def __init__(self) -> None: ...
        @overload
        def __init__(self, body: bytes) -> None: ...

    def __repr__(self) -> str:
        return repr(self.decode())

    def __bytes__(self) -> bytes:
        return self.to_bytes()

    def to_bytes(self) -> bytes:  # pragma: no cover - overridden by concrete classes
        raise NotImplementedError

    def set_timestamp(self, ts1: int, ts2: int) -> None:
        self.timestamp = ts2 | (ts1 << 32)

    def split_timestamp(self) -> Tuple[int, int]:
        ts1 = self.timestamp >> 32
        ts2 = self.timestamp - (ts1 << 32)
        return (ts1, ts2)

    def decode_price(self, price_attr: str) -> float:
        price = getattr(self, price_attr)
        if callable(price):
            raise ValueError(f"Please check the price attribute for {price_attr}")
        return price / (10 ** getattr(self, "price_precision"))

    def decode(self, prefix: str = "") -> Any:
        builtin_attrs = {}
        for attr in dir(self):
            if attr.startswith("__"):
                continue
            value = getattr(self, attr)
            if "price" in attr and attr not in ("price_precision", "decode_price"):
                value = self.decode_price(attr)
            if callable(value):
                continue
            if isinstance(value, (int, float, str, bool)):
                builtin_attrs[attr] = value
            elif isinstance(value, bytes):
                try:
                    builtin_attrs[attr] = value.decode(encoding="ascii").rstrip()
                except UnicodeDecodeError:
                    builtin_attrs[attr] = value
        fields = [(key, type(value)) for key, value in builtin_attrs.items()]
        decoded_class = make_dataclass(f"{prefix}{self.__class__.__name__}", fields)
        return decoded_class(**builtin_attrs)

    def get_attributes(self, call_able: bool = False) -> List[str]:
        attrs = [attr for attr in dir(self) if not attr.startswith("__")]
        return [attr for attr in attrs if callable(getattr(self, attr)) == call_able]


class AddOrderMessage(MarketMessage, metaclass=ABCMeta):
    """Abstract base for the two Add Order message variants."""

    order_reference_number: int
    buy_sell_indicator: bytes
    shares: int
    stock: bytes
    price: int


class ModifyOrderMessage(MarketMessage, metaclass=ABCMeta):
    """Abstract base for messages that modify a resting order."""

    order_reference_number: int


class TradeMessage(MarketMessage, metaclass=ABCMeta):
    """Abstract base for trade messages."""

    match_number: int


__all__ = [
    "AddOrderMessage",
    "MarketMessage",
    "ModifyOrderMessage",
    "TradeMessage",
]
