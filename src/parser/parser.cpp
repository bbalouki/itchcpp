#include "parser.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "messages.hpp"

namespace itch {

namespace {

// Check the system's endianness at compile time (or runtime as a fallback).
// This determines if we need to swap bytes at all.
inline bool is_little_endian() {
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
        char c[4];
    } bint = {0x01020304};
    return bint.c[0] == 4;
#endif
}

// Generic byte-swapping function for any integral type.
template <typename T>
T swap_bytes(T value) {
    static_assert(std::is_integral<T>::value,
                  "swap_bytes can only be used with integral types");
    union {
        T val;
        uint8_t bytes[sizeof(T)];
    } src, dst;
    src.val = value;
    for (size_t i = 0; i < sizeof(T); ++i) {
        dst.bytes[i] = src.bytes[sizeof(T) - 1 - i];
    }
    return dst.val;
}

// Converts a value from big-endian (network order) to the host's native byte
// order. On little-endian systems (like x86/x64), it swaps the bytes. On
// big-endian systems, it does nothing.
template <typename T>
T from_big_endian(T value) {
    if (is_little_endian()) {
        return swap_bytes(value);
    }
    return value;
}

// Helper to unpack a value from a buffer at a given offset
template <typename T>
T unpack(const char* buffer, size_t& offset) {
    T value;
    std::memcpy(&value, buffer + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

// Specialization for char arrays (strings)
void unpack_string(const char* buffer, size_t& offset, char* dest,
                   size_t size) {
    std::memcpy(dest, buffer + offset, size);
    offset += size;
}

// Unpack and convert multi-byte fields from big-endian (network) to host order
template <typename T>
T unpack_be(const char* buffer, size_t& offset) {
    T value;
    std::memcpy(&value, buffer + offset, sizeof(T));
    offset += sizeof(T);
    return from_big_endian(value);
}

// ITCH timestamps are 48-bit big-endian integers.
// 2 bytes for high part, 4 bytes for low part.
uint64_t unpack_timestamp(const char* buffer, size_t& offset) {
    uint16_t high;
    uint32_t low;
    std::memcpy(&high, buffer + offset, sizeof(high));
    offset += sizeof(high);
    std::memcpy(&low, buffer + offset, sizeof(low));
    offset += sizeof(low);
    // Convert each part from big-endian before combining
    return (static_cast<uint64_t>(from_big_endian(high)) << 32) |
           from_big_endian(low);
}

}  // namespace

void unpack_message(SystemEventMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.event_code = unpack<char>(buffer, offset);
}

void unpack_message(StockDirectoryMessage& msg, const char* buffer,
                    size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.market_category = unpack<char>(buffer, offset);
    msg.financial_status_indicator = unpack<char>(buffer, offset);
    msg.round_lot_size = unpack_be<uint32_t>(buffer, offset);
    msg.round_lots_only = unpack<char>(buffer, offset);
    msg.issue_classification = unpack<char>(buffer, offset);
    unpack_string(buffer, offset, msg.issue_sub_type, 2);
    msg.authenticity = unpack<char>(buffer, offset);
    msg.short_sale_threshold_indicator = unpack<char>(buffer, offset);
    msg.ipo_flag = unpack<char>(buffer, offset);
    msg.luld_ref = unpack<char>(buffer, offset);
    msg.etp_flag = unpack<char>(buffer, offset);
    msg.etp_leverage_factor = unpack_be<uint32_t>(buffer, offset);
    msg.inverse_indicator = unpack<char>(buffer, offset);
}

void unpack_message(StockTradingActionMessage& msg, const char* buffer,
                    size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.trading_state = unpack<char>(buffer, offset);
    msg.reserved = unpack<char>(buffer, offset);
    unpack_string(buffer, offset, msg.reason, 4);
}

void unpack_message(RegSHOMessage& msg, const char* buffer, size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.reg_sho_action = unpack<char>(buffer, offset);
}

void unpack_message(MarketParticipantPositionMessage& msg, const char* buffer,
                    size_t& offset) {
    unpack_string(buffer, offset, msg.mpid, 4);
    unpack_string(buffer, offset, msg.stock, 8);
    msg.primary_market_maker = unpack<char>(buffer, offset);
    msg.market_maker_mode = unpack<char>(buffer, offset);
    msg.market_participant_state = unpack<char>(buffer, offset);
}

void unpack_message(MWCBDeclineLevelMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.level1 = unpack_be<uint64_t>(buffer, offset);
    msg.level2 = unpack_be<uint64_t>(buffer, offset);
    msg.level3 = unpack_be<uint64_t>(buffer, offset);
}

void unpack_message(MWCBStatusMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.breached_level = unpack<char>(buffer, offset);
}

void unpack_message(IPOQuotingPeriodUpdateMessage& msg, const char* buffer,
                    size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.ipo_quotation_release_time = unpack_be<uint32_t>(buffer, offset);
    msg.ipo_quotation_release_qualifier = unpack<char>(buffer, offset);
    msg.ipo_price = unpack_be<uint32_t>(buffer, offset);
}

void unpack_message(LULDAuctionCollarMessage& msg, const char* buffer,
                    size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.auction_collar_reference_price = unpack_be<uint32_t>(buffer, offset);
    msg.upper_auction_collar_price = unpack_be<uint32_t>(buffer, offset);
    msg.lower_auction_collar_price = unpack_be<uint32_t>(buffer, offset);
    msg.auction_collar_extension = unpack_be<uint32_t>(buffer, offset);
}

void unpack_message(OperationalHaltMessage& msg, const char* buffer,
                    size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.market_code = unpack<char>(buffer, offset);
    msg.operational_halt_action = unpack<char>(buffer, offset);
}

void unpack_message(AddOrderMessage& msg, const char* buffer, size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.buy_sell_indicator = unpack<char>(buffer, offset);
    msg.shares = unpack_be<uint32_t>(buffer, offset);
    unpack_string(buffer, offset, msg.stock, 8);
    msg.price = unpack_be<uint32_t>(buffer, offset);
}

void unpack_message(AddOrderMPIDAttributionMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.buy_sell_indicator = unpack<char>(buffer, offset);
    msg.shares = unpack_be<uint32_t>(buffer, offset);
    unpack_string(buffer, offset, msg.stock, 8);
    msg.price = unpack_be<uint32_t>(buffer, offset);
    unpack_string(buffer, offset, msg.attribution, 4);
}

void unpack_message(OrderExecutedMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.executed_shares = unpack_be<uint32_t>(buffer, offset);
    msg.match_number = unpack_be<uint64_t>(buffer, offset);
}

void unpack_message(OrderExecutedWithPriceMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.executed_shares = unpack_be<uint32_t>(buffer, offset);
    msg.match_number = unpack_be<uint64_t>(buffer, offset);
    msg.printable = unpack<char>(buffer, offset);
    msg.execution_price = unpack_be<uint32_t>(buffer, offset);
}

void unpack_message(OrderCancelMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.cancelled_shares = unpack_be<uint32_t>(buffer, offset);
}

void unpack_message(OrderDeleteMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
}

void unpack_message(OrderReplaceMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.original_order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.new_order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.shares = unpack_be<uint32_t>(buffer, offset);
    msg.price = unpack_be<uint32_t>(buffer, offset);
}

void unpack_message(NonCrossTradeMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.order_reference_number = unpack_be<uint64_t>(buffer, offset);
    msg.buy_sell_indicator = unpack<char>(buffer, offset);
    msg.shares = unpack_be<uint32_t>(buffer, offset);
    unpack_string(buffer, offset, msg.stock, 8);
    msg.price = unpack_be<uint32_t>(buffer, offset);
    msg.match_number = unpack_be<uint64_t>(buffer, offset);
}

void unpack_message(CrossTradeMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.shares = unpack_be<uint64_t>(buffer, offset);
    unpack_string(buffer, offset, msg.stock, 8);
    msg.cross_price = unpack_be<uint32_t>(buffer, offset);
    msg.match_number = unpack_be<uint64_t>(buffer, offset);
    msg.cross_type = unpack<char>(buffer, offset);
}

void unpack_message(BrokenTradeMessage& msg, const char* buffer,
                    size_t& offset) {
    msg.match_number = unpack_be<uint64_t>(buffer, offset);
}

void unpack_message(NOIIMessage& msg, const char* buffer, size_t& offset) {
    msg.paired_shares = unpack_be<uint64_t>(buffer, offset);
    msg.imbalance_shares = unpack_be<uint64_t>(buffer, offset);
    msg.imbalance_direction = unpack<char>(buffer, offset);
    unpack_string(buffer, offset, msg.stock, 8);
    msg.far_price = unpack_be<uint32_t>(buffer, offset);
    msg.near_price = unpack_be<uint32_t>(buffer, offset);
    msg.current_reference_price = unpack_be<uint32_t>(buffer, offset);
    msg.cross_type = unpack<char>(buffer, offset);
    msg.price_variation_indicator = unpack<char>(buffer, offset);
}

void unpack_message(RetailPriceImprovementIndicatorMessage& msg,
                    const char* buffer, size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.interest_flag = unpack<char>(buffer, offset);
}

void unpack_message(DLCRMessage& msg, const char* buffer, size_t& offset) {
    unpack_string(buffer, offset, msg.stock, 8);
    msg.open_eligibility_status = unpack<char>(buffer, offset);
    msg.minimum_allowable_price = unpack_be<uint32_t>(buffer, offset);
    msg.maximum_allowable_price = unpack_be<uint32_t>(buffer, offset);
    msg.near_execution_price = unpack_be<uint32_t>(buffer, offset);
    msg.near_execution_time = unpack_be<uint64_t>(buffer, offset);
    msg.lower_price_range_collar = unpack_be<uint32_t>(buffer, offset);
    msg.upper_price_range_collar = unpack_be<uint32_t>(buffer, offset);
}

template <typename T>
void Parser::register_handler(char type) {
    m_handlers[type] = [](const char* buffer) -> Message {
        T msg;
        size_t offset = 1;  // Skip message type

        msg.stock_locate = unpack_be<uint16_t>(buffer, offset);
        msg.tracking_number = unpack_be<uint16_t>(buffer, offset);
        msg.timestamp = unpack_timestamp(buffer, offset);

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

void Parser::parse(std::istream& is,
                   const std::function<void(const Message&)>& callback) {
    while (is && is.peek() != EOF) {
        // ITCH 5.0 messages are prefixed with a 2-byte, big-endian length
        // field.
        uint16_t length;
        is.read(reinterpret_cast<char*>(&length), sizeof(length));

        if (is.eof()) {
            break;  // Clean exit at end of file
        }
        if (!is) {
            throw std::runtime_error(
                "Failed to read message length from stream.");
        }

        // The length is in network byte order (big-endian), convert it to host
        // order.
        length = from_big_endian(length);

        if (length == 0) continue;  // Should not happen, but good to be safe

        // Read the full message payload into a buffer.
        std::vector<char> buffer(length);
        if (!is.read(buffer.data(), length)) {
            throw std::runtime_error("Incomplete message at end of stream.");
        }

        // The first byte of the payload is the message type.
        const char message_type = buffer[0];

        // Find the registered handler for this message type.
        auto handler_it = m_handlers.find(message_type);
        if (handler_it != m_handlers.end()) {
            callback(handler_it->second(buffer.data()));
        } else {
            std::cerr << "Warning: Unknown or unhandled message type: "
                      << message_type << '\n';
        }
    }
}

std::vector<Message> Parser::parse(std::istream& is) {
    std::vector<Message> messages;
    parse(is, [&](const Message& msg) { messages.push_back(msg); });
    return messages;
}

}  // namespace itch
