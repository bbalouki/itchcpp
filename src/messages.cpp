#include "messages.hpp"

namespace itch {

void print_impl(std::ostream& out, const SystemEventMessage& msg) {
    out << "System Event:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Event Code: " << msg.event_code;
}

void print_impl(std::ostream& out, const StockDirectoryMessage& msg) {
    out << "Stock Directory:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const StockTradingActionMessage& msg) {
    out << "Stock Trading Action:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
       << "  State: " << msg.trading_state;
}

void print_impl(std::ostream& out, const RegSHOMessage& msg) {
    out << "Reg SHO Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const MarketParticipantPositionMessage& msg) {
    out << "Market Participant Position:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  MPID: " << to_string(msg.mpid, 4) << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const MWCBDeclineLevelMessage& msg) {
    out << "MWCB Decline Level:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Level 1: " << static_cast<double>(msg.level1) / MWCB_PRICE_DIVISOR;
}

void print_impl(std::ostream& out, const MWCBStatusMessage& msg) {
    out << "MWCB Status:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Level: " << msg.breached_level;
}

void print_impl(std::ostream& out, const IPOQuotingPeriodUpdateMessage& msg) {
    out << "IPO Quoting Period Update:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const LULDAuctionCollarMessage& msg) {
    out << "LULD Auction Collar:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const OperationalHaltMessage& msg) {
    out << "Operational Halt:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const AddOrderMessage& msg) {
    out << "Add Order:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
       << "  Side: " << msg.buy_sell_indicator << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / PRICE_DIVISOR;
}

void print_impl(std::ostream& out, const AddOrderMPIDAttributionMessage& msg) {
    out << "Add Order (MPID):\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
       << "  MPID: " << to_string(msg.attribution, 4) << "\n"
       << "  Side: " << msg.buy_sell_indicator << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / PRICE_DIVISOR;
}

void print_impl(std::ostream& out, const OrderExecutedMessage& msg) {
    out << "Order Executed:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number << "\n"
       << "  Shares: " << msg.executed_shares;
}

void print_impl(std::ostream& out, const OrderExecutedWithPriceMessage& msg) {
    out << "Order Executed w/ Price:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number << "\n"
       << "  Price: " << msg.execution_price / PRICE_DIVISOR;
}

void print_impl(std::ostream& out, const OrderCancelMessage& msg) {
    out << "Order Cancel:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number << "\n"
       << "  Cancelled Shares: " << msg.cancelled_shares;
}

void print_impl(std::ostream& out, const OrderDeleteMessage& msg) {
    out << "Order Delete:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Ref#: " << msg.order_reference_number;
}

void print_impl(std::ostream& out, const OrderReplaceMessage& msg) {
    out << "Order Replace:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Original Ref#: " << msg.original_order_reference_number << "\n"
       << "  New Ref#: " << msg.new_order_reference_number << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / PRICE_DIVISOR;
}

void print_impl(std::ostream& out, const NonCrossTradeMessage& msg) {
    out << "Non-Cross Trade:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
       << "  Side: " << msg.buy_sell_indicator << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Price: " << msg.price / PRICE_DIVISOR;
}

void print_impl(std::ostream& out, const CrossTradeMessage& msg) {
    out << "Cross Trade:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
       << "  Shares: " << msg.shares << "\n"
       << "  Cross Price: " << msg.cross_price / PRICE_DIVISOR << "\n"
       << "  Match#: " << msg.match_number << "\n"
       << "  Cross Type: " << msg.cross_type;
}

void print_impl(std::ostream& out, const BrokenTradeMessage& msg) {
    out << "Broken Trade:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Match#: " << msg.match_number;
}

void print_impl(std::ostream& out, const NOIIMessage& msg) {
    out << "NOII Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
       << "  Paired Shares: " << msg.paired_shares << "\n"
       << "  Imbalance Shares: " << msg.imbalance_shares << "\n"
       << "  Imbalance Direction: " << msg.imbalance_direction << "\n"
       << "  Far Price: " << msg.far_price / PRICE_DIVISOR << "\n"
       << "  Near Price: " << msg.near_price / PRICE_DIVISOR << "\n"
       << "  Reference Price: " << msg.current_reference_price / PRICE_DIVISOR << "\n"
       << "  Cross Type: " << msg.cross_type << "\n"
       << "  Price Variation Indicator: " << msg.price_variation_indicator;
}

void print_impl(std::ostream& out, const RetailPriceImprovementIndicatorMessage& msg) {
    out << "RPII Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_impl(std::ostream& out, const DLCRMessage& msg) {
    out << "DLCR Message:\n"
       << "  Timestamp: " << msg.timestamp << "\n"
       << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

void print_message(std::ostream& out, const Message& msg) {
    std::visit([&out](auto&& arg) { print_impl(out, arg); }, msg);
}

std::ostream& operator<<(std::ostream& out, const Message& msg) {
    print_message(out, msg);
    return out;
}

}  // namespace itch
