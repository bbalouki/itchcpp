"""Type stubs for the compiled ``itchcpp._itchcpp`` extension module.

The concrete message classes are implemented in C++. For typing they are presented
as subclasses of the abstract bases in :mod:`itchcpp._bases` (``MarketMessage`` and
friends), matching the runtime ``isinstance`` behavior established by virtual-subclass
registration in :mod:`itchcpp.messages`.
"""

from typing import Any, BinaryIO, Callable, Iterator

from ._bases import (
    AddOrderMessage,
    MarketMessage,
    ModifyOrderMessage,
    TradeMessage,
)

AllMessages: bytes
messages: dict[bytes, type[MarketMessage]]

class SystemEventMessage(MarketMessage):
    event_code: bytes

class StockDirectoryMessage(MarketMessage):
    stock: bytes
    market_category: bytes
    financial_status_indicator: bytes
    round_lot_size: int
    round_lots_only: bytes
    issue_classification: bytes
    issue_sub_type: bytes
    authenticity: bytes
    short_sale_threshold_indicator: bytes
    ipo_flag: bytes
    luld_ref: bytes
    etp_flag: bytes
    etp_leverage_factor: int
    inverse_indicator: bytes

class StockTradingActionMessage(MarketMessage):
    stock: bytes
    trading_state: bytes
    reserved: bytes
    reason: bytes

class RegSHOMessage(MarketMessage):
    stock: bytes
    reg_sho_action: bytes

class MarketParticipantPositionMessage(MarketMessage):
    mpid: bytes
    stock: bytes
    primary_market_maker: bytes
    market_maker_mode: bytes
    market_participant_state: bytes

class MWCBDeclineLeveMessage(MarketMessage):
    level1_price: int
    level2_price: int
    level3_price: int

class MWCBStatusMessage(MarketMessage):
    breached_level: bytes

class IPOQuotingPeriodUpdateMessage(MarketMessage):
    stock: bytes
    ipo_release_time: int
    ipo_release_qualifier: bytes
    ipo_price: int

class LULDAuctionCollarMessage(MarketMessage):
    stock: bytes
    auction_collar_reference_price: int
    upper_auction_collar_price: int
    lower_auction_collar_price: int
    auction_collar_extention: int

class OperationalHaltMessage(MarketMessage):
    stock: bytes
    market_code: bytes
    operational_halt_action: bytes

class AddOrderNoMPIAttributionMessage(AddOrderMessage): ...

class AddOrderMPIDAttribution(AddOrderMessage):
    attribution: bytes

class OrderExecutedMessage(ModifyOrderMessage):
    executed_shares: int
    match_number: int

class OrderExecutedWithPriceMessage(ModifyOrderMessage):
    executed_shares: int
    match_number: int
    printable: bytes
    execution_price: int

class OrderCancelMessage(ModifyOrderMessage):
    cancelled_shares: int

class OrderDeleteMessage(ModifyOrderMessage): ...

class OrderReplaceMessage(ModifyOrderMessage):
    new_order_reference_number: int
    shares: int
    price: int

class NonCrossTradeMessage(TradeMessage):
    order_reference_number: int
    buy_sell_indicator: bytes
    shares: int
    stock: bytes
    price: int

class CrossTradeMessage(TradeMessage):
    shares: int
    stock: bytes
    cross_price: int
    cross_type: bytes

class BrokenTradeMessage(TradeMessage): ...

class NOIIMessage(MarketMessage):
    paired_shares: int
    imbalance_shares: int
    imbalance_direction: bytes
    stock: bytes
    far_price: int
    near_price: int
    current_reference_price: int
    cross_type: bytes
    variation_indicator: bytes

class RetailPriceImprovementIndicator(MarketMessage):
    stock: bytes
    interest_flag: bytes

class DLCRMessage(MarketMessage):
    stock: bytes
    open_eligibility_status: bytes
    minimum_allowable_price: int
    maximum_allowable_price: int
    near_execution_price: int
    near_execution_time: int
    lower_price_range_collar: int
    upper_price_range_collar: int

def create_message(message_type: bytes, **kwargs: Any) -> MarketMessage: ...

class MessageParser:
    message_type: bytes
    def __init__(self, message_type: bytes = ...) -> None: ...
    def get_message_type(self, message: bytes) -> MarketMessage: ...
    def parse_stream(
        self, data: bytes, save_file: BinaryIO | None = ...
    ) -> Iterator[MarketMessage]: ...
    def parse_file(
        self, file: BinaryIO, cachesize: int = ..., save_file: BinaryIO | None = ...
    ) -> Iterator[MarketMessage]: ...
    def parse_messages(
        self, data: bytes | BinaryIO, callback: Callable[[MarketMessage], None]
    ) -> None: ...

class Bbo:
    has_bid: bool
    has_ask: bool
    bid_shares: int
    ask_shares: int
    bid_price: float
    ask_price: float

class L3Book:
    symbol: str
    def bbo(self) -> Bbo: ...

class BookManager:
    def __init__(self) -> None: ...
    def track_symbol(self, symbol: str) -> None: ...
    def process_stream(self, raw_binary_data: bytes) -> None: ...
    def book_count(self) -> int: ...
    def book_for_symbol(self, symbol: str) -> L3Book: ...

class Vwap:
    def __init__(self) -> None: ...
    def add(self, raw_price: int, shares: int) -> None: ...
    def value(self) -> float: ...
    def volume(self) -> int: ...
    def reset(self) -> None: ...
