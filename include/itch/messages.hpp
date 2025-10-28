#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace itch {

// DISABLE PADDING
#pragma pack(push, 1)

struct SystemEventMessage {
    char     message_type = 'S';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     event_code;
};

struct StockDirectoryMessage {
    char     message_type = 'R';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     market_category;
    char     financial_status_indicator;
    uint32_t round_lot_size;
    char     round_lots_only;
    char     issue_classification;
    char     issue_sub_type[2];
    char     authenticity;
    char     short_sale_threshold_indicator;
    char     ipo_flag;
    char     luld_ref;
    char     etp_flag;
    uint32_t etp_leverage_factor;
    char     inverse_indicator;
};

struct StockTradingActionMessage {
    char     message_type = 'H';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     trading_state;
    char     reserved;
    char     reason[4];
};

struct RegSHOMessage {
    char     message_type = 'Y';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     reg_sho_action;
};

struct MarketParticipantPositionMessage {
    char     message_type = 'L';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     mpid[4];
    char     stock[8];
    char     primary_market_maker;
    char     market_maker_mode;
    char     market_participant_state;
};

struct MWCBDeclineLevelMessage {
    char     message_type = 'V';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t level1;
    uint64_t level2;
    uint64_t level3;
};

struct MWCBStatusMessage {
    char     message_type = 'W';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     breached_level;
};

struct IPOQuotingPeriodUpdateMessage {
    char     message_type = 'K';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    uint32_t ipo_quotation_release_time;
    char     ipo_quotation_release_qualifier;
    uint32_t ipo_price;
};

struct LULDAuctionCollarMessage {
    char     message_type = 'J';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    uint32_t auction_collar_reference_price;
    uint32_t upper_auction_collar_price;
    uint32_t lower_auction_collar_price;
    uint32_t auction_collar_extension;
};

struct OperationalHaltMessage {
    char     message_type = 'h';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     market_code;
    char     operational_halt_action;
};

struct AddOrderMessage {
    char     message_type = 'A';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char     buy_sell_indicator;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
};

struct AddOrderMPIDAttributionMessage {
    char     message_type = 'F';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char     buy_sell_indicator;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
    char     attribution[4];
};

struct OrderExecutedMessage {
    char     message_type = 'E';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
};

struct OrderExecutedWithPriceMessage {
    char     message_type = 'C';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
    char     printable;
    uint32_t execution_price;
};

struct OrderCancelMessage {
    char     message_type = 'X';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t cancelled_shares;
};

struct OrderDeleteMessage {
    char     message_type = 'D';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
};

struct OrderReplaceMessage {
    char     message_type = 'U';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t original_order_reference_number;
    uint64_t new_order_reference_number;
    uint32_t shares;
    uint32_t price;
};

struct NonCrossTradeMessage {
    char     message_type = 'P';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char     buy_sell_indicator;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
    uint64_t match_number;
};

struct CrossTradeMessage {
    char     message_type = 'Q';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t shares;
    char     stock[8];
    uint32_t cross_price;
    uint64_t match_number;
    char     cross_type;
};

struct BrokenTradeMessage {
    char     message_type = 'B';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t match_number;
};

struct NOIIMessage {
    char     message_type = 'I';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    char     imbalance_direction;
    char     stock[8];
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_reference_price;
    char     cross_type;
    char     price_variation_indicator;
};

struct RetailPriceImprovementIndicatorMessage {
    char     message_type = 'N';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     interest_flag;
};

struct DLCRMessage {
    char     message_type = 'O';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     open_eligibility_status;
    uint32_t minimum_allowable_price;
    uint32_t maximum_allowable_price;
    uint32_t near_execution_price;
    uint64_t near_execution_time;
    uint32_t lower_price_range_collar;
    uint32_t upper_price_range_collar;
};

// RESTORE DEFAULT PADDING
#pragma pack(pop)

/**
 The TotalView ITCH feed is composed of a series of messages that describe
orders added to, removed from, and executed on Nasdaq as well as disseminate
Cross and Stock Directory information.
Each message begins with a one-byte Message Type field that identifies the
structure of the remainder of the message.  The Message Type is followed by
fields that are specific to each message type.

All Message have the following attributes:
- message_type: A single letter that identify the message
- timestamp: Time at which the message was generated (Nanoseconds past
midnight)
- stock_locate: Locate code identifying the security
- tracking_number: Nasdaq internal tracking number

for more details on each message type, see the
[documentation](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf).

@note
Prices are integers fields supplied with an associated precision.  When
converted to a decimal format, prices are in fixed point format, where the
precision defines the number of decimal places. For example, a field flagged as
Price (4) has an implied 4 decimal places.  The maximum value of price (4) in
TotalView ITCH is 200,000.0000 (decimal, 77359400 hex). ``price_precision`` is
4 for all messages except MWCBDeclineLeveMessage where ``price_precision``
is 8.
*/
using Message =
    std::variant<SystemEventMessage, StockDirectoryMessage, StockTradingActionMessage,
                 RegSHOMessage, MarketParticipantPositionMessage, MWCBDeclineLevelMessage,
                 MWCBStatusMessage, IPOQuotingPeriodUpdateMessage, LULDAuctionCollarMessage,
                 OperationalHaltMessage, AddOrderMessage, AddOrderMPIDAttributionMessage,
                 OrderExecutedMessage, OrderExecutedWithPriceMessage, OrderCancelMessage,
                 OrderDeleteMessage, OrderReplaceMessage, NonCrossTradeMessage, CrossTradeMessage,
                 BrokenTradeMessage, NOIIMessage, RetailPriceImprovementIndicatorMessage,
                 DLCRMessage>;

constexpr int    STOCK_LEN          = 8;
constexpr double PRICE_DIVISOR      = 10000.0;
constexpr double MWCB_PRICE_DIVISOR = 1.0E8;

// Convert char arrays to strings, trimming trailing spaces.
inline std::string to_string(const char* arr, size_t size) {
    size_t len = size;
    while (len > 0 && (arr[len - 1] == ' ' || arr[len - 1] == '\0')) { len--; }
    return std::string{arr, len};
}

void print_impl(std::ostream& out, const SystemEventMessage& msg);
void print_impl(std::ostream& out, const StockDirectoryMessage& msg);
void print_impl(std::ostream& out, const StockTradingActionMessage& msg);
void print_impl(std::ostream& out, const RegSHOMessage& msg);
void print_impl(std::ostream& out, const MarketParticipantPositionMessage& msg);
void print_impl(std::ostream& out, const MWCBDeclineLevelMessage& msg);
void print_impl(std::ostream& out, const MWCBStatusMessage& msg);
void print_impl(std::ostream& out, const IPOQuotingPeriodUpdateMessage& msg);
void print_impl(std::ostream& out, const LULDAuctionCollarMessage& msg);
void print_impl(std::ostream& out, const OperationalHaltMessage& msg);
void print_impl(std::ostream& out, const AddOrderMessage& msg);
void print_impl(std::ostream& out, const AddOrderMPIDAttributionMessage& msg);
void print_impl(std::ostream& out, const OrderExecutedMessage& msg);
void print_impl(std::ostream& out, const OrderExecutedWithPriceMessage& msg);
void print_impl(std::ostream& out, const OrderCancelMessage& msg);
void print_impl(std::ostream& out, const OrderDeleteMessage& msg);
void print_impl(std::ostream& out, const OrderReplaceMessage& msg);
void print_impl(std::ostream& out, const NonCrossTradeMessage& msg);
void print_impl(std::ostream& out, const CrossTradeMessage& msg);
void print_impl(std::ostream& out, const BrokenTradeMessage& msg);
void print_impl(std::ostream& out, const NOIIMessage& msg);
void print_impl(std::ostream& out, const RetailPriceImprovementIndicatorMessage& msg);
void print_impl(std::ostream& out, const DLCRMessage& msg);

// General print function that dispatches to the correct implementation
void print_message(std::ostream& out, const Message& msg);

// print an itch::Message
std::ostream& operator<<(std::ostream& out, const Message& msg);

}  // namespace itch
