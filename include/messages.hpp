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

// Convert char arrays to strings, trimming trailing spaces.
inline std::string to_string(const char* arr, size_t size) {
    size_t len = size;
    while (len > 0 && (arr[len - 1] == ' ' || arr[len - 1] == '\0')) {
        len--;
    }
    return std::string(arr, len);
}

inline void print_impl(std::ostream& os, const SystemEventMessage& msg) {
    os << "System Event:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Event Code: " << msg.event_code;
}

inline void print_impl(std::ostream& os, const StockDirectoryMessage& msg) {
    os << "Stock Directory:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const StockTradingActionMessage& msg) {
    os << "Stock Trading Action:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8) << "\n"
       << "  State: " << msg.trading_state;
}

inline void print_impl(std::ostream& os, const RegSHOMessage& msg) {
    os << "Reg SHO Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const MarketParticipantPositionMessage& msg) {
    os << "Market Participant Position:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  MPID: " << to_string(msg.mpid, 4) << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const MWCBDeclineLevelMessage& msg) {
    os << "MWCB Decline Level:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Level 1: " << msg.level1 / 1.0E8;
}

inline void print_impl(std::ostream& os, const MWCBStatusMessage& msg) {
    os << "MWCB Status:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Level: " << msg.breached_level;
}

inline void print_impl(std::ostream& os, const IPOQuotingPeriodUpdateMessage& msg) {
    os << "IPO Quoting Period Update:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const LULDAuctionCollarMessage& msg) {
    os << "LULD Auction Collar:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const OperationalHaltMessage& msg) {
    os << "Operational Halt:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const AddOrderMessage& msg) {
    os << "Add Order:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8) << "\n"
       << "  Side: " << msg.buy_sell_indicator << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / 10000.0;
}

inline void print_impl(std::ostream& os, const AddOrderMPIDAttributionMessage& msg) {
    os << "Add Order (MPID):\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8) << "\n"
       << "  MPID: " << to_string(msg.attribution, 4) << "\n"
       << "  Side: " << msg.buy_sell_indicator << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / 10000.0;
}

inline void print_impl(std::ostream& os, const OrderExecutedMessage& msg) {
    os << "Order Executed:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number << "\n"
       << "  Shares: " << msg.executed_shares;
}

inline void print_impl(std::ostream& os, const OrderExecutedWithPriceMessage& msg) {
    os << "Order Executed w/ Price:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number << "\n"
       << "  Price: " << msg.execution_price / 10000.0;
}

inline void print_impl(std::ostream& os, const OrderCancelMessage& msg) {
    os << "Order Cancel:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number << "\n"
       << "  Cancelled Shares: " << msg.cancelled_shares;
}

inline void print_impl(std::ostream& os, const OrderDeleteMessage& msg) {
    os << "Order Delete:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number;
}

inline void print_impl(std::ostream& os, const OrderReplaceMessage& msg) {
    os << "Order Replace:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Original Ref#: " << msg.original_order_reference_number << "\n"
       << "  New Ref#: " << msg.new_order_reference_number << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / 10000.0;
}

inline void print_impl(std::ostream& os, const NonCrossTradeMessage& msg) {
    os << "Non-Cross Trade:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8) << "\n"
       << "  Side: " << msg.buy_sell_indicator << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / 10000.0;
}

inline void print_impl(std::ostream& os, const CrossTradeMessage& msg) {
    os << "Cross Trade:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8) << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Cross Price: " << msg.cross_price / 10000.0 << "\n"
       << "  Match#: " << msg.match_number << "\n"
       << "  Cross Type: " << msg.cross_type;
}

inline void print_impl(std::ostream& os, const BrokenTradeMessage& msg) {
    os << "Broken Trade:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Match#: " << msg.match_number;
}

inline void print_impl(std::ostream& os, const NOIIMessage& msg) {
    os << "NOII Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8) << "\n"
       << "  Paired Shares: " << msg.paired_shares << "\n"
       << "  Imbalance Shares: " << msg.imbalance_shares << "\n"
       << "  Imbalance Direction: " << msg.imbalance_direction << "\n"
       << "  Far Price: " << msg.far_price / 10000.0 << "\n"
       << "  Near Price: " << msg.near_price / 10000.0 << "\n"
       << "  Reference Price: " << msg.current_reference_price / 10000.0 << "\n"
       << "  Cross Type: " << msg.cross_type << "\n"
       << "  Price Variation Indicator: " << msg.price_variation_indicator;
}

inline void print_impl(std::ostream& os, const RetailPriceImprovementIndicatorMessage& msg) {
    os << "RPII Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_impl(std::ostream& os, const DLCRMessage& msg) {
    os << "DLCR Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, 8);
}

inline void print_message(std::ostream& os, const Message& msg) {
    std::visit([&os](auto&& arg) { print_impl(os, arg); }, msg);
}

inline std::ostream& operator<<(std::ostream& os, const Message& msg) {
    print_message(os, msg);
    return os;
}

}  // namespace itch
