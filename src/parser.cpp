#include "itch/parser.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "itch/detail/wire.hpp"

namespace itch {

namespace utils {

template <typename ValueType>
auto unpack(const char* buffer, std::size_t& offset) -> ValueType {
    ValueType value;
    std::memcpy(&value, buffer + offset, sizeof(ValueType));
    offset += sizeof(ValueType);

    if constexpr (std::is_integral_v<ValueType> && sizeof(ValueType) > 1) {
        return from_big_endian(value);
    } else {
        return value;
    }
}

inline auto unpack_string(const char* buffer, std::size_t& offset, char* dest, std::size_t size)
    -> void {
    std::memcpy(dest, buffer + offset, size);
    offset += size;
}

inline auto unpack_timestamp(const char* buffer, std::size_t& offset) -> std::uint64_t {
    std::uint16_t high {};
    std::uint32_t low {};
    constexpr int LOWER_SHIFT = 32;
    std::memcpy(&high, buffer + offset, sizeof(high));
    offset += sizeof(high);
    std::memcpy(&low, buffer + offset, sizeof(low));
    offset += sizeof(low);
    high = from_big_endian(high);
    low  = from_big_endian(low);
    return (static_cast<std::uint64_t>(high) << LOWER_SHIFT) | low;
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

namespace {

using detail::WIRE_SIZE;

/// @brief Decodes the common header and type-specific body of a message of
///        type MsgType from a frame, returning it wrapped in the Message variant.
template <typename MsgType>
auto decode_typed(const char* buffer) -> Message {
    MsgType     msg;
    std::size_t offset = 1;  // Skip the message type byte.

    msg.stock_locate    = utils::unpack<std::uint16_t>(buffer, offset);
    msg.tracking_number = utils::unpack<std::uint16_t>(buffer, offset);
    msg.timestamp       = utils::unpack_timestamp(buffer, offset);

    unpack_message(msg, buffer, offset);

    return msg;
}

/// @brief One slot of the flat, type-byte-indexed dispatch table.
struct DispatchEntry {
    std::uint16_t wire_size {0};               ///< Expected on-wire frame size (0 == unknown type).
    Message (*decode)(const char*) {nullptr};  ///< Decoder, or null for an unknown type.
};

constexpr std::size_t DISPATCH_TABLE_SIZE = 256;

/// @brief Builds the dispatch table at compile time so message dispatch is a
///        single flat-array lookup plus a direct (inlinable) call, with no map
///        traversal and no type-erased `std::function` indirection.
consteval auto build_dispatch_table() -> std::array<DispatchEntry, DISPATCH_TABLE_SIZE> {
    std::array<DispatchEntry, DISPATCH_TABLE_SIZE> table {};

    auto add = [&table]<typename MsgType>(char type) {
        table[static_cast<unsigned char>(type)] =
            DispatchEntry {static_cast<std::uint16_t>(WIRE_SIZE<MsgType>), &decode_typed<MsgType>};
    };

    // The canonical message-type registry lives in itch/detail/wire.hpp so the
    // dispatch table, overlay, and encoder all share one definition.
    detail::for_each_message_type(add);

    return table;
}

constexpr auto DISPATCH_TABLE = build_dispatch_table();

}  // namespace

auto Parser::report_error(ParseError error, char message_type) -> void {
    switch (error) {
        case ParseError::unknown_type:
            ++m_unknown_message_count;
            break;
        case ParseError::size_mismatch:
        case ParseError::truncated:
            ++m_malformed_message_count;
            break;
    }
    if (m_error_callback) {
        m_error_callback(error, message_type);
    }
}

auto Parser::parse_impl(const char* data, std::size_t size, const MessageCallback& callback)
    -> std::optional<ParseError> {
    std::size_t offset = 0;
    while (offset < size) {
        // Ensure we can read the 2-byte length prefix.
        if (offset + sizeof(std::uint16_t) > size) {
            report_error(ParseError::truncated, '\0');
            return ParseError::truncated;
        }
        std::uint16_t length {};
        std::memcpy(&length, data + offset, sizeof(length));
        length = utils::from_big_endian(length);
        offset += sizeof(std::uint16_t);

        if (length == 0) {
            continue;  // Skip zero-length padding frames.
        }

        // Ensure the full declared payload is present.
        if (offset + length > size) {
            report_error(ParseError::truncated, '\0');
            return ParseError::truncated;
        }

        const char* message      = data + offset;
        const char  message_type = message[0];
        offset += length;

        const DispatchEntry& entry = DISPATCH_TABLE[static_cast<unsigned char>(message_type)];
        if (entry.decode == nullptr) {
            // Unknown type: skip and count rather than writing to a global stream.
            report_error(ParseError::unknown_type, message_type);
            continue;
        }
        // A frame shorter than the type requires would make the decoder read into
        // the next frame; reject it. A longer frame is tolerated for forward
        // compatibility (the trailing bytes are simply skipped).
        if (length < entry.wire_size) {
            report_error(ParseError::size_mismatch, message_type);
            continue;
        }

        callback(entry.decode(message));
    }
    return std::nullopt;
}

auto Parser::parse(const char* data, size_t size, const MessageCallback& callback) -> void {
    if (parse_impl(data, size, callback).has_value()) {
        throw std::runtime_error("Incomplete message at end of buffer.");
    }
}

constexpr size_t average_message_size = 20;

auto Parser::parse(const char* data, size_t size) -> std::vector<Message> {
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

namespace {
// std::byte and char may legitimately alias; route through void* to obtain the
// char view the framing core works with, without a forbidden reinterpret_cast.
auto as_char_ptr(std::span<const std::byte> data) -> const char* {
    // char may alias std::byte; the house style forbids reinterpret_cast, so the
    // void* hop is the intended form here.
    const void* raw = data.data();
    return static_cast<const char*>(raw);  // NOLINT(bugprone-casting-through-void)
}
}  // namespace

auto Parser::parse(std::span<const std::byte> data, const MessageCallback& callback) -> void {
    parse(as_char_ptr(data), data.size(), callback);
}

auto Parser::parse(std::span<const std::byte> data) -> std::vector<Message> {
    return parse(as_char_ptr(data), data.size());
}

auto Parser::parse(std::span<const std::byte> data, const std::vector<char>& messages)
    -> std::vector<Message> {
    return parse(as_char_ptr(data), data.size(), messages);
}

#ifdef __cpp_lib_expected
auto Parser::try_parse(std::span<const std::byte> data, const MessageCallback& callback)
    -> std::expected<void, ParseError> {
    if (auto error = parse_impl(as_char_ptr(data), data.size(), callback)) {
        return std::unexpected(*error);
    }
    return {};
}

auto Parser::try_parse(std::span<const std::byte> data)
    -> std::expected<std::vector<Message>, ParseError> {
    std::vector<Message> messages;
    messages.reserve(data.size() / average_message_size);
    if (auto error = parse_impl(as_char_ptr(data), data.size(), [&](const Message& msg) {
            messages.push_back(msg);
        })) {
        return std::unexpected(*error);
    }
    return messages;
}
#endif

auto Parser::set_error_callback(ErrorCallback callback) -> void {
    m_error_callback = std::move(callback);
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
