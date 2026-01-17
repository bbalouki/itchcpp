#include "itch/parser.hpp"

#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>

namespace itch {

namespace utils {
inline auto is_little_endian() -> bool {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return true;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return false;
#elif defined(_WIN32)  // Windows is always little-endian
    return true;
#else
    // Runtime check if compile-time check is not available
    const union {
        uint32_t i;
        char     c[4];
    } bint = {0x01020304};
    return bint.c[0] == 4;
#endif
}

template <typename T>
auto from_big_endian(T value) -> T {
    if (is_little_endian()) {
        return swap_bytes(value);
    }
    return value;
}

template <typename T>
auto unpack(const char* buffer, size_t& offset) -> T {
    T value;
    std::memcpy(&value, buffer + offset, sizeof(T));
    offset += sizeof(T);

    if constexpr (std::is_integral_v<T> && sizeof(T) > 1) {
        return from_big_endian(value);
    } else {
        return value;
    }
}

inline auto unpack_string(const char* buffer, size_t& offset, char* dest, size_t size) -> void {
    std::memcpy(dest, buffer + offset, size);
    offset += size;
}

inline auto unpack_timestamp(const char* buffer, size_t& offset) -> uint64_t {
    uint16_t      high {};
    uint32_t      low {};
    constexpr int lower_shift = 32;
    std::memcpy(&high, buffer + offset, sizeof(high));
    offset += sizeof(high);
    std::memcpy(&low, buffer + offset, sizeof(low));
    offset += sizeof(low);
    high = from_big_endian(high);
    low  = from_big_endian(low);
    return (static_cast<uint64_t>(high) << lower_shift) | low;
}
}  // namespace utils

auto unpack_message(SystemEventMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.event_code = utils::unpack<char>(buffer, offset);
}

auto unpack_message(StockDirectoryMessage& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.market_category            = utils::unpack<char>(buffer, offset);
    msg.financial_status_indicator = utils::unpack<char>(buffer, offset);
    msg.round_lot_size             = utils::unpack<uint32_t>(buffer, offset);
    msg.round_lots_only            = utils::unpack<char>(buffer, offset);
    msg.issue_classification       = utils::unpack<char>(buffer, offset);

    utils::unpack_string(buffer, offset, msg.issue_sub_type, 2);
    msg.authenticity                   = utils::unpack<char>(buffer, offset);
    msg.short_sale_threshold_indicator = utils::unpack<char>(buffer, offset);
    msg.ipo_flag                       = utils::unpack<char>(buffer, offset);
    msg.luld_ref                       = utils::unpack<char>(buffer, offset);
    msg.etp_flag                       = utils::unpack<char>(buffer, offset);
    msg.etp_leverage_factor            = utils::unpack<uint32_t>(buffer, offset);
    msg.inverse_indicator              = utils::unpack<char>(buffer, offset);
}

auto unpack_message(StockTradingActionMessage& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.trading_state = utils::unpack<char>(buffer, offset);
    msg.reserved      = utils::unpack<char>(buffer, offset);
    utils::unpack_string(buffer, offset, msg.reason, 4);
}

auto unpack_message(RegSHOMessage& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.reg_sho_action = utils::unpack<char>(buffer, offset);
}

auto unpack_message(MarketParticipantPositionMessage& msg, const char* buffer, size_t& offset)
    -> void {
    utils::unpack_string(buffer, offset, msg.mpid, 4);
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.primary_market_maker     = utils::unpack<char>(buffer, offset);
    msg.market_maker_mode        = utils::unpack<char>(buffer, offset);
    msg.market_participant_state = utils::unpack<char>(buffer, offset);
}

auto unpack_message(MWCBDeclineLevelMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.level1 = utils::unpack<uint64_t>(buffer, offset);
    msg.level2 = utils::unpack<uint64_t>(buffer, offset);
    msg.level3 = utils::unpack<uint64_t>(buffer, offset);
}

auto unpack_message(MWCBStatusMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.breached_level = utils::unpack<char>(buffer, offset);
}

auto unpack_message(IPOQuotingPeriodUpdateMessage& msg, const char* buffer, size_t& offset)
    -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.ipo_quotation_release_time      = utils::unpack<uint32_t>(buffer, offset);
    msg.ipo_quotation_release_qualifier = utils::unpack<char>(buffer, offset);
    msg.ipo_price                       = utils::unpack<uint32_t>(buffer, offset);
}

auto unpack_message(LULDAuctionCollarMessage& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.auction_collar_reference_price = utils::unpack<uint32_t>(buffer, offset);
    msg.upper_auction_collar_price     = utils::unpack<uint32_t>(buffer, offset);
    msg.lower_auction_collar_price     = utils::unpack<uint32_t>(buffer, offset);
    msg.auction_collar_extension       = utils::unpack<uint32_t>(buffer, offset);
}

auto unpack_message(OperationalHaltMessage& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.market_code             = utils::unpack<char>(buffer, offset);
    msg.operational_halt_action = utils::unpack<char>(buffer, offset);
}

auto unpack_message(AddOrderMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.buy_sell_indicator     = utils::unpack<char>(buffer, offset);
    msg.shares                 = utils::unpack<uint32_t>(buffer, offset);
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.price = utils::unpack<uint32_t>(buffer, offset);
}

auto unpack_message(AddOrderMPIDAttributionMessage& msg, const char* buffer, size_t& offset)
    -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.buy_sell_indicator     = utils::unpack<char>(buffer, offset);
    msg.shares                 = utils::unpack<uint32_t>(buffer, offset);

    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.price = utils::unpack<uint32_t>(buffer, offset);
    utils::unpack_string(buffer, offset, msg.attribution, 4);
}

auto unpack_message(OrderExecutedMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.executed_shares        = utils::unpack<uint32_t>(buffer, offset);
    msg.match_number           = utils::unpack<uint64_t>(buffer, offset);
}

auto unpack_message(OrderExecutedWithPriceMessage& msg, const char* buffer, size_t& offset)
    -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.executed_shares        = utils::unpack<uint32_t>(buffer, offset);
    msg.match_number           = utils::unpack<uint64_t>(buffer, offset);
    msg.printable              = utils::unpack<char>(buffer, offset);
    msg.execution_price        = utils::unpack<uint32_t>(buffer, offset);
}

auto unpack_message(OrderCancelMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.cancelled_shares       = utils::unpack<uint32_t>(buffer, offset);
}

auto unpack_message(OrderDeleteMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
}

auto unpack_message(OrderReplaceMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.original_order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.new_order_reference_number      = utils::unpack<uint64_t>(buffer, offset);
    msg.shares                          = utils::unpack<uint32_t>(buffer, offset);
    msg.price                           = utils::unpack<uint32_t>(buffer, offset);
}

auto unpack_message(NonCrossTradeMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.order_reference_number = utils::unpack<uint64_t>(buffer, offset);
    msg.buy_sell_indicator     = utils::unpack<char>(buffer, offset);
    msg.shares                 = utils::unpack<uint32_t>(buffer, offset);

    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);

    msg.price        = utils::unpack<uint32_t>(buffer, offset);
    msg.match_number = utils::unpack<uint64_t>(buffer, offset);
}

auto unpack_message(CrossTradeMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.shares = utils::unpack<uint64_t>(buffer, offset);
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);

    msg.cross_price  = utils::unpack<uint32_t>(buffer, offset);
    msg.match_number = utils::unpack<uint64_t>(buffer, offset);
    msg.cross_type   = utils::unpack<char>(buffer, offset);
}

auto unpack_message(BrokenTradeMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.match_number = utils::unpack<uint64_t>(buffer, offset);
}

auto unpack_message(NOIIMessage& msg, const char* buffer, size_t& offset) -> void {
    msg.paired_shares       = utils::unpack<uint64_t>(buffer, offset);
    msg.imbalance_shares    = utils::unpack<uint64_t>(buffer, offset);
    msg.imbalance_direction = utils::unpack<char>(buffer, offset);

    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);

    msg.far_price                 = utils::unpack<uint32_t>(buffer, offset);
    msg.near_price                = utils::unpack<uint32_t>(buffer, offset);
    msg.current_reference_price   = utils::unpack<uint32_t>(buffer, offset);
    msg.cross_type                = utils::unpack<char>(buffer, offset);
    msg.price_variation_indicator = utils::unpack<char>(buffer, offset);
}

using RPIMsg = RetailPriceImprovementIndicatorMessage;
auto unpack_message(RPIMsg& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.interest_flag = utils::unpack<char>(buffer, offset);
}

auto unpack_message(DLCRMessage& msg, const char* buffer, size_t& offset) -> void {
    utils::unpack_string(buffer, offset, msg.stock, STOCK_LEN);
    msg.open_eligibility_status  = utils::unpack<char>(buffer, offset);
    msg.minimum_allowable_price  = utils::unpack<uint32_t>(buffer, offset);
    msg.maximum_allowable_price  = utils::unpack<uint32_t>(buffer, offset);
    msg.near_execution_price     = utils::unpack<uint32_t>(buffer, offset);
    msg.near_execution_time      = utils::unpack<uint64_t>(buffer, offset);
    msg.lower_price_range_collar = utils::unpack<uint32_t>(buffer, offset);
    msg.upper_price_range_collar = utils::unpack<uint32_t>(buffer, offset);
}

template <typename T>
auto Parser::register_handler(char type) -> void {
    m_handlers[type] = [](const char* buffer) -> Message {
        T      msg;
        size_t offset = 1;  // Skip message type

        msg.stock_locate    = utils::unpack<uint16_t>(buffer, offset);
        msg.tracking_number = utils::unpack<uint16_t>(buffer, offset);
        msg.timestamp       = utils::unpack_timestamp(buffer, offset);

        unpack_message(msg, buffer, offset);

        return msg;
    };
}

Parser::Parser() {
    register_handler<SystemEventMessage>('S');
    register_handler<StockDirectoryMessage>('R');
    register_handler<StockTradingActionMessage>('H');
    register_handler<RegSHOMessage>('Y');
    register_handler<MarketParticipantPositionMessage>('L');
    register_handler<MWCBDeclineLevelMessage>('V');
    register_handler<MWCBStatusMessage>('W');
    register_handler<IPOQuotingPeriodUpdateMessage>('K');
    register_handler<LULDAuctionCollarMessage>('J');
    register_handler<OperationalHaltMessage>('h');
    register_handler<AddOrderMessage>('A');
    register_handler<AddOrderMPIDAttributionMessage>('F');
    register_handler<OrderExecutedMessage>('E');
    register_handler<OrderExecutedWithPriceMessage>('C');
    register_handler<OrderCancelMessage>('X');
    register_handler<OrderDeleteMessage>('D');
    register_handler<OrderReplaceMessage>('U');
    register_handler<NonCrossTradeMessage>('P');
    register_handler<CrossTradeMessage>('Q');
    register_handler<BrokenTradeMessage>('B');
    register_handler<NOIIMessage>('I');
    register_handler<RetailPriceImprovementIndicatorMessage>('N');
    register_handler<DLCRMessage>('O');
}

auto Parser::parse(const char* data, size_t size, const MessageCallback& callback) -> void {
    size_t offset = 0;
    while (offset < size) {
        // Ensure we can read the length field
        if (offset + sizeof(uint16_t) > size) {
            throw std::runtime_error("Incomplete message header at end of buffer.");
        }
        uint16_t length {};
        std::memcpy(&length, data + offset, sizeof(length));
        length = utils::from_big_endian(length);
        offset += sizeof(uint16_t);

        if (length == 0) {
            continue;
        }

        // Ensure the full message payload is available
        if (offset + length > size) {
            throw std::runtime_error("Incomplete message at end of buffer.");
        }
        const char* message      = data + offset;
        const char  message_type = message[0];

        auto handler_it = m_handlers.find(message_type);
        if (handler_it != m_handlers.end()) {
            callback(handler_it->second(message));
        } else {
            std::cerr << "Unknown or unhandled message type: " << message_type << '\n';
        }
        offset += length;
    }
}

constexpr size_t average_message_size = 20;
auto             Parser::parse(const char* data, size_t size) -> std::vector<Message> {
    std::vector<Message> messages;
    messages.reserve(size / average_message_size);
    parse(data, size, [&](const Message& msg) { messages.push_back(msg); });
    return messages;
}

auto Parser::parse(const char* data, size_t size, const std::vector<char>& messages)
    -> std::vector<Message> {
    std::vector<Message> results;
    std::set<char>       filter(messages.begin(), messages.end());

    if (filter.empty()) {
        return results;
    }
    results.reserve(size / average_message_size);

    auto callback = [&](const Message& msg) {
        char message_type = std::visit([](auto&& arg) { return arg.message_type; }, msg);
        if (filter.contains(message_type)) {
            results.push_back(msg);
        }
    };
    parse(data, size, callback);
    return results;
}

// Read the whole stream into a buffer
static auto read_stream_into_buffer(std::istream& data) -> std::vector<char> {
    data.seekg(0, std::ios::end);
    auto size = data.tellg();
    data.seekg(0, std::ios::beg);

    if (size < 0) {
        throw std::runtime_error("Failed to determine stream size.");
    }

    std::vector<char> buffer(static_cast<size_t>(size));
    data.read(buffer.data(), size);
    return buffer;
}

auto Parser::parse(std::istream& data, const MessageCallback& callback) -> void {
    auto buffer = read_stream_into_buffer(data);
    parse(buffer.data(), buffer.size(), callback);
}

auto Parser::parse(std::istream& data) -> std::vector<Message> {
    auto buffer = read_stream_into_buffer(data);
    return parse(buffer.data(), buffer.size());
}

auto Parser::parse(std::istream& data, const std::vector<char>& messages) -> std::vector<Message> {
    auto buffer = read_stream_into_buffer(data);
    return parse(buffer.data(), buffer.size(), messages);
}

}  // namespace itch
