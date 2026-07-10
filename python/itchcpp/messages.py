"""Message classes for the native ITCH backend.

Mirrors ``itch.messages`` from the pure-Python ``itchfeed`` package: the same
concrete message classes (re-exported from the compiled ``_itchcpp`` extension),
the ``messages`` registry, the ``AllMessages`` constant, and the ``create_message``
factory. The abstract base classes (:class:`MarketMessage`, :class:`AddOrderMessage`,
:class:`ModifyOrderMessage`, :class:`TradeMessage`) come from :mod:`itchcpp._bases`
and the native classes are registered here as virtual subclasses so ``isinstance``
checks behave exactly as they do upstream.
"""

from __future__ import annotations

from ._bases import (
    AddOrderMessage,
    MarketMessage,
    ModifyOrderMessage,
    TradeMessage,
)
from ._itchcpp import (  # noqa: F401  (re-exported)
    AddOrderMPIDAttribution,
    AddOrderNoMPIAttributionMessage,
    AllMessages,
    BrokenTradeMessage,
    CrossTradeMessage,
    DLCRMessage,
    IPOQuotingPeriodUpdateMessage,
    LULDAuctionCollarMessage,
    MarketParticipantPositionMessage,
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
    create_message,
    messages,
)


# Register the native classes as virtual subclasses for isinstance/issubclass parity.
_ADD_ORDER = (AddOrderNoMPIAttributionMessage, AddOrderMPIDAttribution)
_MODIFY_ORDER = (
    OrderExecutedMessage,
    OrderExecutedWithPriceMessage,
    OrderCancelMessage,
    OrderDeleteMessage,
    OrderReplaceMessage,
)
_TRADE = (NonCrossTradeMessage, CrossTradeMessage, BrokenTradeMessage)

for _cls in messages.values():
    MarketMessage.register(_cls)
for _cls in _ADD_ORDER:
    AddOrderMessage.register(_cls)
for _cls in _MODIFY_ORDER:
    ModifyOrderMessage.register(_cls)
for _cls in _TRADE:
    TradeMessage.register(_cls)


__all__ = [
    "AddOrderMessage",
    "AddOrderMPIDAttribution",
    "AddOrderNoMPIAttributionMessage",
    "AllMessages",
    "BrokenTradeMessage",
    "CrossTradeMessage",
    "DLCRMessage",
    "IPOQuotingPeriodUpdateMessage",
    "LULDAuctionCollarMessage",
    "MarketMessage",
    "MarketParticipantPositionMessage",
    "ModifyOrderMessage",
    "MWCBDeclineLeveMessage",
    "MWCBStatusMessage",
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
    "create_message",
    "messages",
]
