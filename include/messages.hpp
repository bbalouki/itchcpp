#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace itch {

// Convert char arrays to strings, trimming trailing spaces.
inline std::string char_array_to_string(const char* arr, size_t size) {
    size_t len = size;
    while (len > 0 && (arr[len - 1] == ' ' || arr[len - 1] == '\0')) {
        len--;
    }
    return std::string(arr, len);
}

struct MarketMessage {
    virtual ~MarketMessage() = default;
    virtual void print(std::ostream& os) const = 0;

    friend std::ostream& operator<<(std::ostream& os,
                                    const MarketMessage& msg) {
        msg.print(os);
        return os;
    }
};

struct SystemEventMessage : MarketMessage {
    char message_type = 'S';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char event_code;

    void print(std::ostream& os) const override {
        os << "System Event:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Event Code: " << event_code;
    }
};

struct StockDirectoryMessage : MarketMessage {
    char message_type = 'R';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char market_category;
    char financial_status_indicator;
    uint32_t round_lot_size;
    char round_lots_only;
    char issue_classification;
    char issue_sub_type[2];
    char authenticity;
    char short_sale_threshold_indicator;
    char ipo_flag;
    char luld_ref;
    char etp_flag;
    uint32_t etp_leverage_factor;
    char inverse_indicator;

    void print(std::ostream& os) const override {
        os << "Stock Directory:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct StockTradingActionMessage : MarketMessage {
    char message_type = 'H';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char trading_state;
    char reserved;
    char reason[4];

    void print(std::ostream& os) const override {
        os << "Stock Trading Action:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8) << "\n"
           << "  State: " << trading_state;
    }
};

struct RegSHOMessage : MarketMessage {
    char message_type = 'Y';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char reg_sho_action;

    void print(std::ostream& os) const override {
        os << "Reg SHO Message:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct MarketParticipantPositionMessage : MarketMessage {
    char message_type = 'L';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char mpid[4];
    char stock[8];
    char primary_market_maker;
    char market_maker_mode;
    char market_participant_state;

    void print(std::ostream& os) const override {
        os << "Market Participant Position:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  MPID: " << char_array_to_string(mpid, 4) << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct MWCBDeclineLevelMessage : MarketMessage {
    char message_type = 'V';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t level1;
    uint64_t level2;
    uint64_t level3;

    void print(std::ostream& os) const override {
        os << "MWCB Decline Level:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Level 1: " << level1 / 1.0E8;
    }
};

struct MWCBStatusMessage : MarketMessage {
    char message_type = 'W';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char breached_level;

    void print(std::ostream& os) const override {
        os << "MWCB Status:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Level: " << breached_level;
    }
};

struct IPOQuotingPeriodUpdateMessage : MarketMessage {
    char message_type = 'K';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    uint32_t ipo_quotation_release_time;
    char ipo_quotation_release_qualifier;
    uint32_t ipo_price;

    void print(std::ostream& os) const override {
        os << "IPO Quoting Period Update:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct LULDAuctionCollarMessage : MarketMessage {
    char message_type = 'J';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    uint32_t auction_collar_reference_price;
    uint32_t upper_auction_collar_price;
    uint32_t lower_auction_collar_price;
    uint32_t auction_collar_extension;

    void print(std::ostream& os) const override {
        os << "LULD Auction Collar:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct OperationalHaltMessage : MarketMessage {
    char message_type = 'h';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char market_code;
    char operational_halt_action;

    void print(std::ostream& os) const override {
        os << "Operational Halt:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct AddOrderMessage : MarketMessage {
    char message_type = 'A';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;

    void print(std::ostream& os) const override {
        os << "Add Order:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8) << "\n"
           << "  Side: " << buy_sell_indicator << "\n"
           << "  Shares: " << shares << "\n"
           << "  Price: " << price / 10000.0;
    }
};

struct AddOrderMPIDAttributionMessage : MarketMessage {
    char message_type = 'F';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;
    char attribution[4];

    void print(std::ostream& os) const override {
        os << "Add Order (MPID):\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8) << "\n"
           << "  MPID: " << char_array_to_string(attribution, 4) << "\n"
           << "  Side: " << buy_sell_indicator << "\n"
           << "  Shares: " << shares << "\n"
           << "  Price: " << price / 10000.0;
    }
};

struct OrderExecutedMessage : MarketMessage {
    char message_type = 'E';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;

    void print(std::ostream& os) const override {
        os << "Order Executed:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Ref#: " << order_reference_number << "\n"
           << "  Shares: " << executed_shares;
    }
};

struct OrderExecutedWithPriceMessage : MarketMessage {
    char message_type = 'C';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
    char printable;
    uint32_t execution_price;

    void print(std::ostream& os) const override {
        os << "Order Executed w/ Price:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Ref#: " << order_reference_number << "\n"
           << "  Price: " << execution_price / 10000.0;
    }
};

struct OrderCancelMessage : MarketMessage {
    char message_type = 'X';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t cancelled_shares;

    void print(std::ostream& os) const override {
        os << "Order Cancel:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Ref#: " << order_reference_number << "\n"
           << "  Cancelled Shares: " << cancelled_shares;
    }
};

struct OrderDeleteMessage : MarketMessage {
    char message_type = 'D';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;

    void print(std::ostream& os) const override {
        os << "Order Delete:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Ref#: " << order_reference_number;
    }
};

struct OrderReplaceMessage : MarketMessage {
    char message_type = 'U';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t original_order_reference_number;
    uint64_t new_order_reference_number;
    uint32_t shares;
    uint32_t price;

    void print(std::ostream& os) const override {
        os << "Order Replace:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Original Ref#: " << original_order_reference_number << "\n"
           << "  New Ref#: " << new_order_reference_number << "\n"
           << "  Shares: " << shares << "\n"
           << "  Price: " << price / 10000.0;
    }
};

struct NonCrossTradeMessage : MarketMessage {
    char message_type = 'P';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;
    uint64_t match_number;

    void print(std::ostream& os) const override {
        os << "Non-Cross Trade:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8) << "\n"
           << "  Side: " << buy_sell_indicator << "\n"
           << "  Shares: " << shares << "\n"
           << "  Price: " << price / 10000.0;
    }
};

struct CrossTradeMessage : MarketMessage {
    char message_type = 'Q';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t shares;
    char stock[8];
    uint32_t cross_price;
    uint64_t match_number;
    char cross_type;

    void print(std::ostream& os) const override {
        os << "Cross Trade:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8) << "\n"
           << "  Shares: " << shares << "\n"
           << "  Cross Price: " << cross_price / 10000.0 << "\n"
           << "  Match#: " << match_number << "\n"
           << "  Cross Type: " << cross_type;
    }
};

struct BrokenTradeMessage : MarketMessage {
    char message_type = 'B';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t match_number;

    void print(std::ostream& os) const override {
        os << "Broken Trade:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Match#: " << match_number;
    }
};

struct NOIIMessage : MarketMessage {
    char message_type = 'I';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    char imbalance_direction;
    char stock[8];
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_reference_price;
    char cross_type;
    char price_variation_indicator;

    void print(std::ostream& os) const override {
        os << "NOII Message:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8) << "\n"
           << "  Paired Shares: " << paired_shares << "\n"
           << "  Imbalance Shares: " << imbalance_shares << "\n"
           << "  Imbalance Direction: " << imbalance_direction << "\n"
           << "  Far Price: " << far_price / 10000.0 << "\n"
           << "  Near Price: " << near_price / 10000.0 << "\n"
           << "  Reference Price: " << current_reference_price / 10000.0 << "\n"
           << "  Cross Type: " << cross_type << "\n"
           << "  Price Variation Indicator: " << price_variation_indicator;
    }
};

struct RetailPriceImprovementIndicatorMessage : MarketMessage {
    char message_type = 'N';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char interest_flag;

    void print(std::ostream& os) const override {
        os << "RPII Message:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

struct DLCRMessage : MarketMessage {
    char message_type = 'O';
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char stock[8];
    char open_eligibility_status;
    uint32_t minimum_allowable_price;
    uint32_t maximum_allowable_price;
    uint32_t near_execution_price;
    uint64_t near_execution_time;
    uint32_t lower_price_range_collar;
    uint32_t upper_price_range_collar;

    void print(std::ostream& os) const override {
        os << "DLCR Message:\n"
           << "  Timestamp: " << timestamp << "\n"
           << "  Stock: " << char_array_to_string(stock, 8);
    }
};

}  // namespace itch
