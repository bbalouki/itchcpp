#include "itch/messages.hpp"

namespace itch {

auto print_impl(std::ostream& out, const SystemEventMessage& msg) -> void {
    out << "System Event:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Event Code: " << msg.event_code;
}

auto print_impl(std::ostream& out, const StockDirectoryMessage& msg) -> void {
    out << "Stock Directory:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const StockTradingActionMessage& msg) -> void {
    out << "Stock Trading Action:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
        << "  State: " << msg.trading_state;
}

auto print_impl(std::ostream& out, const RegSHOMessage& msg) -> void {
    out << "Reg SHO Message:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const MarketParticipantPositionMessage& msg) -> void {
    out << "Market Participant Position:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  MPID: " << to_string(msg.mpid, 4) << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const MWCBDeclineLevelMessage& msg) -> void {
    out << "MWCB Decline Level:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Level 1: " << static_cast<double>(msg.level1) / MWCB_PRICE_DIVISOR;
}

auto print_impl(std::ostream& out, const MWCBStatusMessage& msg) -> void {
    out << "MWCB Status:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Level: " << msg.breached_level;
}

auto print_impl(std::ostream& out, const IPOQuotingPeriodUpdateMessage& msg) -> void {
    out << "IPO Quoting Period Update:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const LULDAuctionCollarMessage& msg) -> void {
    out << "LULD Auction Collar:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const OperationalHaltMessage& msg) -> void {
    out << "Operational Halt:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const AddOrderMessage& msg) -> void {
    out << "Add Order:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
        << "  Side: " << msg.buy_sell_indicator << "\n"
        << "  Shares: " << msg.shares << "\n"
        << "  Price: " << msg.price / PRICE_DIVISOR;
}

auto print_impl(std::ostream& out, const AddOrderMPIDAttributionMessage& msg) -> void {
    out << "Add Order (MPID):\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
        << "  MPID: " << to_string(msg.attribution, 4) << "\n"
        << "  Side: " << msg.buy_sell_indicator << "\n"
        << "  Shares: " << msg.shares << "\n"
        << "  Price: " << msg.price / PRICE_DIVISOR;
}

auto print_impl(std::ostream& out, const OrderExecutedMessage& msg) -> void {
    out << "Order Executed:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Ref#: " << msg.order_reference_number << "\n"
        << "  Shares: " << msg.executed_shares;
}

auto print_impl(std::ostream& out, const OrderExecutedWithPriceMessage& msg) -> void {
    out << "Order Executed w/ Price:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Ref#: " << msg.order_reference_number << "\n"
        << "  Price: " << msg.execution_price / PRICE_DIVISOR;
}

auto print_impl(std::ostream& out, const OrderCancelMessage& msg) -> void {
    out << "Order Cancel:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Ref#: " << msg.order_reference_number << "\n"
        << "  Cancelled Shares: " << msg.cancelled_shares;
}

auto print_impl(std::ostream& out, const OrderDeleteMessage& msg) -> void {
    out << "Order Delete:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Ref#: " << msg.order_reference_number;
}

auto print_impl(std::ostream& out, const OrderReplaceMessage& msg) -> void {
    out << "Order Replace:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Original Ref#: " << msg.original_order_reference_number << "\n"
        << "  New Ref#: " << msg.new_order_reference_number << "\n"
        << "  Shares: " << msg.shares << "\n"
        << "  Price: " << msg.price / PRICE_DIVISOR;
}

auto print_impl(std::ostream& out, const NonCrossTradeMessage& msg) -> void {
    out << "Non-Cross Trade:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
        << "  Side: " << msg.buy_sell_indicator << "\n"
        << "  Shares: " << msg.shares << "\n"
        << "  Price: " << msg.price / PRICE_DIVISOR;
}

auto print_impl(std::ostream& out, const CrossTradeMessage& msg) -> void {
    out << "Cross Trade:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN) << "\n"
        << "  Shares: " << msg.shares << "\n"
        << "  Cross Price: " << msg.cross_price / PRICE_DIVISOR << "\n"
        << "  Match#: " << msg.match_number << "\n"
        << "  Cross Type: " << msg.cross_type;
}

auto print_impl(std::ostream& out, const BrokenTradeMessage& msg) -> void {
    out << "Broken Trade:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Match#: " << msg.match_number;
}

auto print_impl(std::ostream& out, const NOIIMessage& msg) -> void {
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

auto print_impl(std::ostream& out, const RetailPriceImprovementIndicatorMessage& msg) -> void {
    out << "RPII Message:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_impl(std::ostream& out, const DLCRMessage& msg) -> void {
    out << "DLCR Message:\n"
        << "  Timestamp: " << msg.timestamp << "\n"
        << "  Stock: " << to_string(msg.stock, STOCK_LEN);
}

auto print_message(std::ostream& out, const Message& msg) -> void {
    std::visit([&out](auto&& arg) { print_impl(out, arg); }, msg);
}

auto operator<<(std::ostream& out, const Message& msg) -> std::ostream& {
    print_message(out, msg);
    return out;
}

}  // namespace itch
