"""itchcpp: a native (C++) NASDAQ TotalView-ITCH 5.0 backend.

``itchcpp`` is a fast, drop-in backend for the pure-Python ``itch`` (PyPI:
``itchfeed``) package. It mirrors that package's public API so existing code only
needs to change the import root, e.g.::

    from itch.parser import MessageParser      # pure-Python
    from itchcpp.parser import MessageParser   # native backend

The same message classes, ``MessageParser`` semantics, ``MarketMessage`` helpers
(``decode`` / ``decode_price`` / ``to_bytes`` / ``set_timestamp`` /
``split_timestamp`` / ``get_attributes``), the ``messages`` registry, the
``AllMessages`` constant, ``create_message``, and the :mod:`itchcpp.indicators`
tables are provided. The :mod:`itchcpp.book` and :mod:`itchcpp.analytics` submodules
add native extras (order-book reconstruction and VWAP) that are not part of the
pure-Python API.
"""

from __future__ import annotations

from importlib.metadata import PackageNotFoundError, version

from . import analytics, book, indicators, messages, parser
from .messages import (
    AddOrderMessage,
    AddOrderMPIDAttribution,
    AddOrderNoMPIAttributionMessage,
    AllMessages,
    BrokenTradeMessage,
    CrossTradeMessage,
    DLCRMessage,
    IPOQuotingPeriodUpdateMessage,
    LULDAuctionCollarMessage,
    MarketMessage,
    MarketParticipantPositionMessage,
    ModifyOrderMessage,
    MWCBDeclineLeveMessage,
    MWCBStatusMessage,
    NOIIMessage,
    NonCrossTradeMessage,
    OperationalHaltMessage,
    OrderCancelMessage,
    OrderDeleteMessage,
    OrderExecutedMessage,
    OrderExecutedWithPriceMessage,
    OrderReplaceMessage,
    RegSHOMessage,
    RetailPriceImprovementIndicator,
    StockDirectoryMessage,
    StockTradingActionMessage,
    SystemEventMessage,
    TradeMessage,
    create_message,
    messages as message_registry,
)
from .parser import MessageParser

try:
    __version__ = version("itchcpp")
except PackageNotFoundError:  # pragma: no cover - source checkout without install
    __version__ = "unknown"

__all__ = [
    "AddOrderMPIDAttribution",
    "AddOrderMessage",
    "AddOrderNoMPIAttributionMessage",
    "AllMessages",
    "BrokenTradeMessage",
    "CrossTradeMessage",
    "DLCRMessage",
    "IPOQuotingPeriodUpdateMessage",
    "LULDAuctionCollarMessage",
    "MWCBDeclineLeveMessage",
    "MWCBStatusMessage",
    "MarketMessage",
    "MarketParticipantPositionMessage",
    "MessageParser",
    "ModifyOrderMessage",
    "NOIIMessage",
    "NonCrossTradeMessage",
    "OperationalHaltMessage",
    "OrderCancelMessage",
    "OrderDeleteMessage",
    "OrderExecutedMessage",
    "OrderExecutedWithPriceMessage",
    "OrderReplaceMessage",
    "RegSHOMessage",
    "RetailPriceImprovementIndicator",
    "StockDirectoryMessage",
    "StockTradingActionMessage",
    "SystemEventMessage",
    "TradeMessage",
    "__version__",
    "analytics",
    "book",
    "create_message",
    "indicators",
    "message_registry",
    "messages",
    "parser",
]
