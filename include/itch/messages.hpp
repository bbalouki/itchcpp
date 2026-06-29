#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace itch {

// DISABLE PADDING
#pragma pack(push, 1)

/// @brief System Event (`S`): signals a market or data-feed handler event.
///
/// Used to signal a market or data feed handler event. The `event_code` marks the
/// major points of the trading day: start and end of the messages stream, of
/// system hours, and of market hours. Consumers use it to bracket a session and
/// to know when the book is expected to be live.
struct SystemEventMessage {
    char     message_type = 'S';  ///< Always 'S'.
    uint16_t stock_locate;        ///< Locate code; 0 for system-wide events.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    char     event_code;          ///< 'O','S','Q','M','E','C' (see indicators::SYSTEM_EVENT_CODES).
};

/// @brief Stock Directory (`R`): the trading and listing reference data for a
///        security, disseminated at the start of each trading day.
///
/// Disseminated for all active Nasdaq-traded symbols at the start of each trading
/// day. Market-data redistributors use it to populate fields such as the
/// Financial Status Indicator and Market Category so each security is classified
/// and displayed correctly. A security not named in any Stock Directory message
/// should not appear in later messages for that day.
struct StockDirectoryMessage {
    char     message_type = 'R';            ///< Always 'R'.
    uint16_t stock_locate;                  ///< Locate code identifying the security.
    uint16_t tracking_number;               ///< Nasdaq internal tracking number.
    uint64_t timestamp;                     ///< Nanoseconds past midnight.
    char     stock[8];                      ///< Stock symbol, right padded with spaces.
    char     market_category;               ///< Listing market (see indicators::MARKET_CATEGORY).
    char     financial_status_indicator;    ///< Financial status of the issuer.
    uint32_t round_lot_size;                ///< Number of shares in a round lot.
    char     round_lots_only;               ///< 'Y' if only round lots may be entered, else 'N'.
    char     issue_classification;          ///< Security class (see indicators::ISSUE_CLASSIFICATION_VALUES).
    char     issue_sub_type[2];             ///< Security sub-type (see indicators::ISSUE_SUB_TYPE_VALUES).
    char     authenticity;                  ///< 'P' production / 'T' test security.
    char     short_sale_threshold_indicator; ///< Reg SHO threshold: 'Y','N',' '.
    char     ipo_flag;                       ///< 'Y' if a new IPO, 'N' if not, ' ' if not available.
    char     luld_ref;                       ///< LULD reference price tier.
    char     etp_flag;                       ///< 'Y' if an exchange-traded product, else 'N'.
    uint32_t etp_leverage_factor;            ///< Leverage factor of the ETP (if applicable).
    char     inverse_indicator;              ///< 'Y' if the ETP is an inverse product.
};

/// @brief Stock Trading Action (`H`): a change in the trading status of a
///        security (halted, paused, quotation-only, or trading).
///
/// An administrative message conveying the current trading status of a security.
/// It is sent before the open and again whenever the status changes, such as a
/// halt, a pause, a release for quotation, or a resumption of trading. The
/// `reason` code explains why the action occurred.
struct StockTradingActionMessage {
    char     message_type = 'H';  ///< Always 'H'.
    uint16_t stock_locate;        ///< Locate code identifying the security.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    char     stock[8];            ///< Stock symbol, right padded with spaces.
    char     trading_state;       ///< 'H','P','Q','T' (see indicators::TRADING_STATES).
    char     reserved;            ///< Reserved.
    char     reason[4];           ///< Trading-action reason code (see indicators::TRADING_ACTION_REASON_CODES).
};

/// @brief Reg SHO Short Sale Price Test Restriction (`Y`): the Reg SHO short-sale
///        restriction state for a security.
///
/// Informs recipients of the SEC Rule 201 (Regulation SHO) short-sale price-test
/// restriction status for a security. It is sent both as a pre-open spin for every
/// security and intraday whenever the restriction status changes.
struct RegSHOMessage {
    char     message_type = 'Y';  ///< Always 'Y'.
    uint16_t stock_locate;        ///< Locate code identifying the security.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    char     stock[8];            ///< Stock symbol, right padded with spaces.
    char     reg_sho_action;      ///< '0' no restriction, '1' restriction in effect, '2' remains.
};

/// @brief Market Participant Position (`L`): a market participant's (market
///        maker's) status in a security.
///
/// Disseminated at the start of the trading day and whenever a status changes. It
/// provides, per firm registered in an issue, the Primary Market Maker status, the
/// Market Maker mode, and the Market Participant state.
struct MarketParticipantPositionMessage {
    char     message_type = 'L';        ///< Always 'L'.
    uint16_t stock_locate;              ///< Locate code identifying the security.
    uint16_t tracking_number;           ///< Nasdaq internal tracking number.
    uint64_t timestamp;                 ///< Nanoseconds past midnight.
    char     mpid[4];                   ///< Market participant identifier.
    char     stock[8];                  ///< Stock symbol, right padded with spaces.
    char     primary_market_maker;      ///< 'Y' if the primary market maker, else 'N'.
    char     market_maker_mode;         ///< Quotation mode (see indicators::MARKET_MAKER_MODE).
    char     market_participant_state;  ///< State (see indicators::MARKET_PARTICIPANT_STATE).
};

/// @brief MWCB Decline Level (`V`): the Market-Wide Circuit Breaker breach points
///        for the day.
///
/// Informs recipients what the daily Market-Wide Circuit Breaker breach points are
/// set to for the current trading day. The three levels correspond to the 5%, 13%,
/// and 20% S&P 500 decline thresholds that trigger a market-wide trading halt.
///
/// @note Unlike every other price field (4 implied decimals), the three level
///       prices here carry 8 implied decimals; use `MwcbPrice` to interpret them.
struct MWCBDeclineLevelMessage {
    char     message_type = 'V';  ///< Always 'V'.
    uint16_t stock_locate;        ///< Locate code; 0 for this market-wide message.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    uint64_t level1;              ///< Level 1 (5%) breach value, 8 implied decimals.
    uint64_t level2;              ///< Level 2 (13%) breach value, 8 implied decimals.
    uint64_t level3;              ///< Level 3 (20%) breach value, 8 implied decimals.
};

/// @brief MWCB Status (`W`): notification that a Market-Wide Circuit Breaker level
///        has been breached.
///
/// Informs recipients when a Market-Wide Circuit Breaker has breached one of the
/// established decline levels, which triggers the corresponding market-wide halt.
struct MWCBStatusMessage {
    char     message_type = 'W';  ///< Always 'W'.
    uint16_t stock_locate;        ///< Locate code; 0 for this market-wide message.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    char     breached_level;      ///< '1', '2', or '3' for the level breached.
};

/// @brief IPO Quoting Period Update (`K`): the anticipated IPO quotation release
///        time and price for a security.
///
/// Indicates the anticipated IPO quotation release time for a security. A
/// cancellation or postponement of the IPO is signalled by setting both the
/// release time and the price to zero.
struct IPOQuotingPeriodUpdateMessage {
    char     message_type = 'K';              ///< Always 'K'.
    uint16_t stock_locate;                    ///< Locate code identifying the security.
    uint16_t tracking_number;                 ///< Nasdaq internal tracking number.
    uint64_t timestamp;                       ///< Nanoseconds past midnight.
    char     stock[8];                        ///< Stock symbol, right padded with spaces.
    uint32_t ipo_quotation_release_time;      ///< Seconds past midnight of the anticipated release.
    char     ipo_quotation_release_qualifier; ///< 'A' anticipated, 'C' cancelled/postponed.
    uint32_t ipo_price;                       ///< IPO price (4 implied decimals).
};

/// @brief LULD Auction Collar (`J`): the auction collar thresholds for a security
///        in a Limit-Up Limit-Down trading pause.
///
/// Indicates the auction collar thresholds within which a paused security may
/// reopen following a Limit-Up Limit-Down (LULD) trading pause.
struct LULDAuctionCollarMessage {
    char     message_type = 'J';             ///< Always 'J'.
    uint16_t stock_locate;                   ///< Locate code identifying the security.
    uint16_t tracking_number;                ///< Nasdaq internal tracking number.
    uint64_t timestamp;                      ///< Nanoseconds past midnight.
    char     stock[8];                       ///< Stock symbol, right padded with spaces.
    uint32_t auction_collar_reference_price; ///< Reference price for the collars (4 decimals).
    uint32_t upper_auction_collar_price;     ///< Upper auction collar price (4 decimals).
    uint32_t lower_auction_collar_price;     ///< Lower auction collar price (4 decimals).
    uint32_t auction_collar_extension;       ///< Number of collar extensions so far.
};

/// @brief Operational Halt (`h`): an operational halt or resumption for a security
///        on a specific market center.
///
/// Indicates the operational status of a security: a service interruption that
/// affects only the designated market center. This differs from a Stock Trading
/// Action: an operational halt is a venue-level interruption rather than a
/// regulatory or volatility trading halt.
struct OperationalHaltMessage {
    char     message_type = 'h';      ///< Always 'h'.
    uint16_t stock_locate;            ///< Locate code identifying the security.
    uint16_t tracking_number;         ///< Nasdaq internal tracking number.
    uint64_t timestamp;               ///< Nanoseconds past midnight.
    char     stock[8];                ///< Stock symbol, right padded with spaces.
    char     market_code;             ///< 'Q' Nasdaq, 'B' BX, 'X' PSX.
    char     operational_halt_action; ///< 'H' halted, 'T' resumed.
};

/// @brief Add Order, No MPID Attribution (`A`): a new displayable order has been
///        accepted and placed on the book.
///
/// Indicates that Nasdaq has accepted a new, unattributed order and added it to
/// the displayable book with a day-unique order reference number. That reference
/// number is used by every subsequent execute, cancel, delete, and replace
/// message that acts on the order.
struct AddOrderMessage {
    char     message_type = 'A';     ///< Always 'A'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Day-unique reference number for the order.
    char     buy_sell_indicator;     ///< 'B' buy, 'S' sell.
    uint32_t shares;                 ///< Displayed share quantity.
    char     stock[8];               ///< Stock symbol, right padded with spaces.
    uint32_t price;                  ///< Display price (4 implied decimals).
};

/// @brief Add Order, With MPID Attribution (`F`): like Add Order, but attributed
///        to a market participant.
///
/// Indicates that Nasdaq has accepted a new attributed order or quotation and
/// added it to the displayable book. It is identical to an Add Order message
/// except that it also carries the attributing market participant identifier.
struct AddOrderMPIDAttributionMessage {
    char     message_type = 'F';     ///< Always 'F'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Day-unique reference number for the order.
    char     buy_sell_indicator;     ///< 'B' buy, 'S' sell.
    uint32_t shares;                 ///< Displayed share quantity.
    char     stock[8];               ///< Stock symbol, right padded with spaces.
    uint32_t price;                  ///< Display price (4 implied decimals).
    char     attribution[4];         ///< Market participant identifier (MPID).
};

/// @brief Order Executed (`E`): a resting order was executed in whole or in part
///        at its display price.
///
/// Sent when an order on the book is executed in whole or in part at its display
/// price. Several of these may be sent for the same order reference number; their
/// effects are cumulative, and the order is removed from the book once it is fully
/// executed.
struct OrderExecutedMessage {
    char     message_type = 'E';     ///< Always 'E'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Reference number of the executed order.
    uint32_t executed_shares;        ///< Number of shares executed.
    uint64_t match_number;           ///< Day-unique match number for the execution.
};

/// @brief Order Executed With Price (`C`): a resting order was executed at a price
///        different from its display price (and may be non-printable).
///
/// Sent when an order on the book executes at a price different from its initial
/// display price, so the execution price is carried explicitly. The execution may
/// be marked non-printable, in which case it is excluded from the time-and-sales
/// tape (typically to be printed later in aggregate).
struct OrderExecutedWithPriceMessage {
    char     message_type = 'C';     ///< Always 'C'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Reference number of the executed order.
    uint32_t executed_shares;        ///< Number of shares executed.
    uint64_t match_number;           ///< Day-unique match number for the execution.
    char     printable;              ///< 'Y' if the trade is printable to the tape, else 'N'.
    uint32_t execution_price;        ///< Price at which the order executed (4 decimals).
};

/// @brief Order Cancel (`X`): a partial cancellation reduced the shares of a
///        resting order.
///
/// Sent when an order on the book is modified by a partial cancellation: the
/// specified number of shares is removed from the order's display size while the
/// order itself remains on the book. A full cancellation is conveyed by an Order
/// Delete (`D`) message instead.
struct OrderCancelMessage {
    char     message_type = 'X';     ///< Always 'X'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Reference number of the cancelled order.
    uint32_t cancelled_shares;       ///< Number of shares cancelled.
};

/// @brief Order Delete (`D`): a resting order was cancelled in its entirety and
///        removed from the book.
///
/// Sent when an order on the book is cancelled in full. All remaining shares
/// become inaccessible and the order must be removed from the book.
struct OrderDeleteMessage {
    char     message_type = 'D';     ///< Always 'D'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Reference number of the deleted order.
};

/// @brief Order Replace (`U`): a resting order was replaced with a new order at a
///        new reference number, size, and/or price (same side and security).
///
/// Sent when an order on the book is cancel-replaced. The original order's shares
/// become inaccessible, and the replacement is assigned a new reference number
/// that is used for all subsequent updates. The side and security are unchanged;
/// only the price and size may differ.
struct OrderReplaceMessage {
    char     message_type = 'U';              ///< Always 'U'.
    uint16_t stock_locate;                    ///< Locate code identifying the security.
    uint16_t tracking_number;                 ///< Nasdaq internal tracking number.
    uint64_t timestamp;                       ///< Nanoseconds past midnight.
    uint64_t original_order_reference_number; ///< Reference number being replaced.
    uint64_t new_order_reference_number;      ///< New reference number for the order.
    uint32_t shares;                          ///< New displayed share quantity.
    uint32_t price;                           ///< New display price (4 implied decimals).
};

/// @brief Trade, Non-Cross (`P`): an execution of a non-displayable order. It does
///        not affect the visible book but is disseminated as a print.
///
/// Provides execution details for normal match events involving non-displayable
/// order types, transmitted when such an order executes in whole or in part.
/// Because the order was never on the displayable book, this message does not
/// change book state; it exists so that consumers can include these executions in
/// the trade tape and volume calculations.
struct NonCrossTradeMessage {
    char     message_type = 'P';     ///< Always 'P'.
    uint16_t stock_locate;           ///< Locate code identifying the security.
    uint16_t tracking_number;        ///< Nasdaq internal tracking number.
    uint64_t timestamp;              ///< Nanoseconds past midnight.
    uint64_t order_reference_number; ///< Reference number of the non-displayed order.
    char     buy_sell_indicator;     ///< 'B' buy, 'S' sell.
    uint32_t shares;                 ///< Number of shares traded.
    char     stock[8];               ///< Stock symbol, right padded with spaces.
    uint32_t price;                  ///< Trade price (4 implied decimals).
    uint64_t match_number;           ///< Day-unique match number for the trade.
};

/// @brief Cross Trade (`Q`): the result of a cross (opening, closing, halt/IPO
///        cross) for a security.
///
/// Indicates that Nasdaq has completed the cross process for a security. It is
/// sent following the Opening Cross, the Closing Cross, and Extended Market Close
/// (EMC) cross events, and reports the matched volume and the single cross price.
struct CrossTradeMessage {
    char     message_type = 'Q';  ///< Always 'Q'.
    uint16_t stock_locate;        ///< Locate code identifying the security.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    uint64_t shares;              ///< Number of shares matched in the cross.
    char     stock[8];            ///< Stock symbol, right padded with spaces.
    uint32_t cross_price;         ///< Price at which the cross executed (4 decimals).
    uint64_t match_number;        ///< Day-unique match number for the cross.
    char     cross_type;          ///< 'O' open, 'C' close, 'H' halt/IPO, 'I' intraday.
};

/// @brief Broken Trade (`B`): a previously disseminated execution has been broken
///        and should be removed from any trade record.
///
/// Sent whenever an execution on Nasdaq is broken under the clearly-erroneous
/// execution policy. A trade break is final and cannot be reinstated; consumers
/// should remove the referenced match number from their trade records.
struct BrokenTradeMessage {
    char     message_type = 'B';  ///< Always 'B'.
    uint16_t stock_locate;        ///< Locate code identifying the security.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    uint64_t match_number;        ///< Match number of the execution being broken.
};

/// @brief Net Order Imbalance Indicator (`I`): the imbalance and price-discovery
///        data disseminated during the opening and closing crosses.
///
/// Disseminates Net Order Imbalance Indicator data at regular intervals during the
/// cross sessions. It reports the paired and imbalanced share quantities and the
/// hypothetical clearing prices (far, near, and current reference price) the cross
/// would produce, so participants can react before the cross executes.
struct NOIIMessage {
    char     message_type = 'I';        ///< Always 'I'.
    uint16_t stock_locate;              ///< Locate code identifying the security.
    uint16_t tracking_number;           ///< Nasdaq internal tracking number.
    uint64_t timestamp;                 ///< Nanoseconds past midnight.
    uint64_t paired_shares;             ///< Shares paired at the current reference price.
    uint64_t imbalance_shares;          ///< Shares not paired (the imbalance).
    char     imbalance_direction;       ///< 'B' buy, 'S' sell, 'N' none, 'O' insufficient, 'P' paused.
    char     stock[8];                  ///< Stock symbol, right padded with spaces.
    uint32_t far_price;                 ///< Cross price using only eligible interest (4 decimals).
    uint32_t near_price;                ///< Cross price using all interest (4 decimals).
    uint32_t current_reference_price;   ///< Price the cross would occur at now (4 decimals).
    char     cross_type;                ///< 'O' open, 'C' close, 'H' halt/IPO cross.
    char     price_variation_indicator; ///< Variation band (see indicators::PRICE_VARIATION_INDICATOR).
};

/// @brief Retail Price Improvement Indicator (`N`): the presence of retail price
///        improvement interest on the bid, the offer, or both.
///
/// Identifies the presence of a retail price improvement interest indication (on
/// the bid, the offer, both, or none) for a Nasdaq-listed security. It signals
/// available retail liquidity without disclosing its size or price.
struct RetailPriceImprovementIndicatorMessage {
    char     message_type = 'N';  ///< Always 'N'.
    uint16_t stock_locate;        ///< Locate code identifying the security.
    uint16_t tracking_number;     ///< Nasdaq internal tracking number.
    uint64_t timestamp;           ///< Nanoseconds past midnight.
    char     stock[8];            ///< Stock symbol, right padded with spaces.
    char     interest_flag;       ///< 'B' bid, 'A' ask, 'C' both, 'N' none.
};

/// @brief Direct Listing with Capital Raise Price Discovery (`O`): price-discovery
///        data for a Direct Listing with a Capital Raise (DLCR) security.
///
/// Disseminated only for Direct Listing with Capital Raise (DLCR) securities, once
/// the security passes its volatility test. It provides the allowable price
/// thresholds, the price range collars, and the anticipated execution price and
/// time used during the DLCR opening.
struct DLCRMessage {
    char     message_type = 'O';      ///< Always 'O'.
    uint16_t stock_locate;            ///< Locate code identifying the security.
    uint16_t tracking_number;         ///< Nasdaq internal tracking number.
    uint64_t timestamp;               ///< Nanoseconds past midnight.
    char     stock[8];                ///< Stock symbol, right padded with spaces.
    char     open_eligibility_status; ///< Whether the security is eligible to open.
    uint32_t minimum_allowable_price; ///< Lowest allowable cross price (4 decimals).
    uint32_t maximum_allowable_price; ///< Highest allowable cross price (4 decimals).
    uint32_t near_execution_price;    ///< Anticipated cross price (4 decimals).
    uint64_t near_execution_time;     ///< Time of the anticipated cross (ns past midnight).
    uint32_t lower_price_range_collar; ///< Lower price range collar (4 decimals).
    uint32_t upper_price_range_collar; ///< Upper price range collar (4 decimals).
};

// RESTORE DEFAULT PADDING
#pragma pack(pop)

/// The TotalView ITCH feed is composed of a series of messages that describe
/// orders added to, removed from, and executed on Nasdaq as well as disseminate
/// Cross and Stock Directory information.
/// Each message begins with a one-byte Message Type field that identifies the
/// structure of the remainder of the message.  The Message Type is followed by
/// fields that are specific to each message type.
///
/// All Message have the following attributes:
/// - message_type: A single letter that identify the message
/// - timestamp: Time at which the message was generated (Nanoseconds past
/// midnight)
/// - stock_locate: Locate code identifying the security
/// - tracking_number: Nasdaq internal tracking number
///
/// for more details on each message type, see the
/// [documentation](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf).
///
/// @note
/// Prices are integers fields supplied with an associated precision.  When
/// converted to a decimal format, prices are in fixed point format, where the
/// precision defines the number of decimal places. For example, a field flagged as
/// Price (4) has an implied 4 decimal places.  The maximum value of price (4) in
/// TotalView ITCH is 200,000.0000 (decimal, 77359400 hex). ``price_precision`` is
/// 4 for all messages except MWCBDeclineLeveMessage where ``price_precision``
/// is 8.
using Message = std::variant<
    SystemEventMessage,
    StockDirectoryMessage,
    StockTradingActionMessage,
    RegSHOMessage,
    MarketParticipantPositionMessage,
    MWCBDeclineLevelMessage,
    MWCBStatusMessage,
    IPOQuotingPeriodUpdateMessage,
    LULDAuctionCollarMessage,
    OperationalHaltMessage,
    AddOrderMessage,
    AddOrderMPIDAttributionMessage,
    OrderExecutedMessage,
    OrderExecutedWithPriceMessage,
    OrderCancelMessage,
    OrderDeleteMessage,
    OrderReplaceMessage,
    NonCrossTradeMessage,
    CrossTradeMessage,
    BrokenTradeMessage,
    NOIIMessage,
    RetailPriceImprovementIndicatorMessage,
    DLCRMessage>;

constexpr int    STOCK_LEN          = 8;
constexpr double PRICE_DIVISOR      = 10000.0;
constexpr double MWCB_PRICE_DIVISOR = 1.0E8;

// Convert char arrays to strings, trimming trailing spaces.
inline auto to_string(const char* source, size_t size) -> std::string {
    size_t len = size;
    while (len > 0 && (source[len - 1] == ' ' || source[len - 1] == '\0')) {
        len--;
    }
    return std::string {source, len};
}

auto print_impl(std::ostream& out, const SystemEventMessage& msg) -> void;
auto print_impl(std::ostream& out, const StockDirectoryMessage& msg) -> void;
auto print_impl(std::ostream& out, const StockTradingActionMessage& msg) -> void;
auto print_impl(std::ostream& out, const RegSHOMessage& msg) -> void;
auto print_impl(std::ostream& out, const MarketParticipantPositionMessage& msg) -> void;
auto print_impl(std::ostream& out, const MWCBDeclineLevelMessage& msg) -> void;
auto print_impl(std::ostream& out, const MWCBStatusMessage& msg) -> void;
auto print_impl(std::ostream& out, const IPOQuotingPeriodUpdateMessage& msg) -> void;
auto print_impl(std::ostream& out, const LULDAuctionCollarMessage& msg) -> void;
auto print_impl(std::ostream& out, const OperationalHaltMessage& msg) -> void;
auto print_impl(std::ostream& out, const AddOrderMessage& msg) -> void;
auto print_impl(std::ostream& out, const AddOrderMPIDAttributionMessage& msg) -> void;
auto print_impl(std::ostream& out, const OrderExecutedMessage& msg) -> void;
auto print_impl(std::ostream& out, const OrderExecutedWithPriceMessage& msg) -> void;
auto print_impl(std::ostream& out, const OrderCancelMessage& msg) -> void;
auto print_impl(std::ostream& out, const OrderDeleteMessage& msg) -> void;
auto print_impl(std::ostream& out, const OrderReplaceMessage& msg) -> void;
auto print_impl(std::ostream& out, const NonCrossTradeMessage& msg) -> void;
auto print_impl(std::ostream& out, const CrossTradeMessage& msg) -> void;
auto print_impl(std::ostream& out, const BrokenTradeMessage& msg) -> void;
auto print_impl(std::ostream& out, const NOIIMessage& msg) -> void;
auto print_impl(std::ostream& out, const RetailPriceImprovementIndicatorMessage& msg) -> void;
auto print_impl(std::ostream& out, const DLCRMessage& msg) -> void;

// General print function that dispatches to the correct implementation
auto print_message(std::ostream& out, const Message& msg) -> void;

// print an itch::Message
auto operator<<(std::ostream& out, const Message& msg) -> std::ostream&;

}  // namespace itch
