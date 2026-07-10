#pragma once

/// @file
/// @brief Zero-copy typed views over raw ITCH message frames.
///
/// Provides `MessageView` and a family of per-message-type `*View` subclasses
/// that read fields lazily, converting each field from network byte order on
/// access rather than eagerly decoding into a host-order struct. This contrasts
/// with the eager `Parser` in parser.hpp, which decodes every field up front.
///
/// @author Bertin Balouki SIMYELI

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <string_view>

#include "itch/detail/wire.hpp"
#include "itch/parser.hpp"

namespace itch::overlay {

namespace detail {

/// @brief Reads an integral field of width sizeof(T) at `offset`, converting from
///        network (big-endian) order on access.
///
/// @tparam FieldType The type of the field to read.
/// @param base Pointer to the start of the message frame.
/// @param offset Byte offset of the field within the frame.
/// @return The field value, converted from big-endian if it is a multi-byte
///         integral type.
template <typename FieldType>
[[nodiscard]] inline auto read_field(const std::byte* base, std::size_t offset) noexcept
    -> FieldType {
    FieldType value {};
    std::memcpy(&value, base + offset, sizeof(FieldType));
    if constexpr (std::is_integral_v<FieldType> && sizeof(FieldType) > 1) {
        return utils::from_big_endian(value);
    } else {
        return value;
    }
}

/// @brief Reads the 48-bit ITCH timestamp at `offset` into a 64-bit value.
///
/// @param base Pointer to the start of the message frame.
/// @param offset Byte offset of the timestamp field within the frame.
/// @return The timestamp, in nanoseconds past midnight.
[[nodiscard]] inline auto read_timestamp(const std::byte* base, std::size_t offset) noexcept
    -> std::uint64_t {
    const auto    high        = read_field<std::uint16_t>(base, offset);
    const auto    low         = read_field<std::uint32_t>(base, offset + 2);
    constexpr int LOWER_SHIFT = 32;
    return (static_cast<std::uint64_t>(high) << LOWER_SHIFT) | low;
}

/// @brief Views an 8-byte fixed-width stock field as a string_view (untrimmed).
///
/// @param base Pointer to the start of the message frame.
/// @param offset Byte offset of the stock field within the frame.
/// @return An untrimmed view of the 8-byte stock symbol field.
[[nodiscard]] inline auto read_stock(const std::byte* base, std::size_t offset) noexcept
    -> std::string_view {
    const void* raw = base + offset;
    return std::string_view {static_cast<const char*>(raw), 8};
}

// Common field offsets shared by every message after the 1-byte type.
constexpr std::size_t STOCK_LOCATE_OFFSET    = 1;
constexpr std::size_t TRACKING_NUMBER_OFFSET = 3;
constexpr std::size_t TIMESTAMP_OFFSET       = 5;

}  // namespace detail

/// @brief A zero-copy typed view over one raw ITCH frame.
///
/// In contrast to the eager `Parser`, which decodes every field of a message into
/// a host-order struct, an overlay view holds only a pointer to the frame in the
/// original buffer and converts each field from network byte order lazily, on the
/// access. For callers that touch only a few fields per message (for example a
/// filter that reads just the stock and price), this avoids decoding the fields
/// they never look at. The view is non-owning: it is valid only while the
/// underlying buffer lives.
class MessageView {
   public:
    /// @brief Constructs an empty view with no underlying frame.
    constexpr MessageView() noexcept = default;

    /// @brief Constructs a view over a raw ITCH message frame.
    ///
    /// @param data Pointer to the start of the frame (the type byte).
    /// @param size Size of the frame in bytes.
    explicit MessageView(const std::byte* data, std::size_t size) noexcept
        : m_data {data}, m_size {size} {}

    /// @brief The one-byte message type.
    /// @return The one-byte message type.
    [[nodiscard]] auto type() const noexcept -> char {
        return static_cast<char>(static_cast<unsigned char>(m_data[0]));
    }

    /// @brief The locate code identifying the security.
    /// @return The locate code identifying the security.
    [[nodiscard]] auto stock_locate() const noexcept -> std::uint16_t {
        return detail::read_field<std::uint16_t>(m_data, detail::STOCK_LOCATE_OFFSET);
    }

    /// @brief The Nasdaq internal tracking number.
    /// @return The Nasdaq internal tracking number.
    [[nodiscard]] auto tracking_number() const noexcept -> std::uint16_t {
        return detail::read_field<std::uint16_t>(m_data, detail::TRACKING_NUMBER_OFFSET);
    }

    /// @brief The message timestamp (nanoseconds past midnight).
    /// @return The message timestamp (nanoseconds past midnight).
    [[nodiscard]] auto timestamp() const noexcept -> std::uint64_t {
        return detail::read_timestamp(m_data, detail::TIMESTAMP_OFFSET);
    }

    /// @brief The raw frame size in bytes.
    /// @return The raw frame size in bytes.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return m_size; }

    /// @brief Pointer to the raw frame (the type byte).
    /// @return Pointer to the raw frame (the type byte).
    [[nodiscard]] auto data() const noexcept -> const std::byte* { return m_data; }

    /// @brief Reads an arbitrary integral field at a byte offset, big-endian.
    ///
    /// @tparam FieldType The type of the field to read.
    /// @param offset Byte offset of the field within the frame.
    /// @return The field value, converted from big-endian if it is a multi-byte
    ///         integral type.
    template <typename FieldType>
    [[nodiscard]] auto read(std::size_t offset) const noexcept -> FieldType {
        return detail::read_field<FieldType>(m_data, offset);
    }

   protected:
    const std::byte* m_data {nullptr};
    std::size_t      m_size {0};
};

/// @brief Lazy view of an Add Order (`A`) message.
class AddOrderView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Day-unique reference number for the order.
    /// @return Day-unique reference number for the order.
    [[nodiscard]] auto order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }

    /// @brief 'B' buy, 'S' sell.
    /// @return 'B' buy, 'S' sell.
    [[nodiscard]] auto buy_sell_indicator() const noexcept -> char { return read<char>(19); }

    /// @brief Displayed share quantity.
    /// @return Displayed share quantity.
    [[nodiscard]] auto shares() const noexcept -> std::uint32_t { return read<std::uint32_t>(20); }

    /// @brief Stock symbol, right padded with spaces.
    /// @return Stock symbol, right padded with spaces.
    [[nodiscard]] auto stock() const noexcept -> std::string_view {
        return detail::read_stock(m_data, 24);
    }

    /// @brief Display price (4 implied decimals).
    /// @return Display price (4 implied decimals).
    [[nodiscard]] auto price() const noexcept -> std::uint32_t { return read<std::uint32_t>(32); }
};

/// @brief Lazy view of an Order Executed (`E`) message.
class OrderExecutedView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Reference number of the executed order.
    /// @return Reference number of the executed order.
    [[nodiscard]] auto order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }

    /// @brief Number of shares executed.
    /// @return Number of shares executed.
    [[nodiscard]] auto executed_shares() const noexcept -> std::uint32_t {
        return read<std::uint32_t>(19);
    }

    /// @brief Day-unique match number for the execution.
    /// @return Day-unique match number for the execution.
    [[nodiscard]] auto match_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(23);
    }
};

/// @brief Lazy view of an Order Executed With Price (`C`) message.
class OrderExecutedWithPriceView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Reference number of the executed order.
    /// @return Reference number of the executed order.
    [[nodiscard]] auto order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }

    /// @brief Number of shares executed.
    /// @return Number of shares executed.
    [[nodiscard]] auto executed_shares() const noexcept -> std::uint32_t {
        return read<std::uint32_t>(19);
    }

    /// @brief Day-unique match number for the execution.
    /// @return Day-unique match number for the execution.
    [[nodiscard]] auto match_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(23);
    }

    /// @brief 'Y' if the trade is printable to the tape, else 'N'.
    /// @return 'Y' if the trade is printable to the tape, else 'N'.
    [[nodiscard]] auto printable() const noexcept -> char { return read<char>(31); }

    /// @brief Price at which the order executed (4 decimals).
    /// @return Price at which the order executed (4 decimals).
    [[nodiscard]] auto execution_price() const noexcept -> std::uint32_t {
        return read<std::uint32_t>(32);
    }
};

/// @brief Lazy view of an Order Cancel (`X`) message.
class OrderCancelView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Reference number of the cancelled order.
    /// @return Reference number of the cancelled order.
    [[nodiscard]] auto order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }

    /// @brief Number of shares cancelled.
    /// @return Number of shares cancelled.
    [[nodiscard]] auto cancelled_shares() const noexcept -> std::uint32_t {
        return read<std::uint32_t>(19);
    }
};

/// @brief Lazy view of an Order Delete (`D`) message.
class OrderDeleteView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Reference number of the deleted order.
    /// @return Reference number of the deleted order.
    [[nodiscard]] auto order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }
};

/// @brief Lazy view of an Order Replace (`U`) message.
class OrderReplaceView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Reference number being replaced.
    /// @return Reference number being replaced.
    [[nodiscard]] auto original_order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }

    /// @brief New reference number for the order.
    /// @return New reference number for the order.
    [[nodiscard]] auto new_order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(19);
    }

    /// @brief New displayed share quantity.
    /// @return New displayed share quantity.
    [[nodiscard]] auto shares() const noexcept -> std::uint32_t { return read<std::uint32_t>(27); }

    /// @brief New display price (4 implied decimals).
    /// @return New display price (4 implied decimals).
    [[nodiscard]] auto price() const noexcept -> std::uint32_t { return read<std::uint32_t>(31); }
};

/// @brief Lazy view of a Trade (`P`, non-cross) message.
class NonCrossTradeView : public MessageView {
   public:
    using MessageView::MessageView;

    /// @brief Reference number of the non-displayed order.
    /// @return Reference number of the non-displayed order.
    [[nodiscard]] auto order_reference_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(11);
    }

    /// @brief 'B' buy, 'S' sell.
    /// @return 'B' buy, 'S' sell.
    [[nodiscard]] auto buy_sell_indicator() const noexcept -> char { return read<char>(19); }

    /// @brief Number of shares traded.
    /// @return Number of shares traded.
    [[nodiscard]] auto shares() const noexcept -> std::uint32_t { return read<std::uint32_t>(20); }

    /// @brief Stock symbol, right padded with spaces.
    /// @return Stock symbol, right padded with spaces.
    [[nodiscard]] auto stock() const noexcept -> std::string_view {
        return detail::read_stock(m_data, 24);
    }

    /// @brief Trade price (4 implied decimals).
    /// @return Trade price (4 implied decimals).
    [[nodiscard]] auto price() const noexcept -> std::uint32_t { return read<std::uint32_t>(32); }

    /// @brief Day-unique match number for the trade.
    /// @return Day-unique match number for the trade.
    [[nodiscard]] auto match_number() const noexcept -> std::uint64_t {
        return read<std::uint64_t>(36);
    }
};

/// @brief The signature for the overlay framing callback.
using ViewCallback = std::function<void(const MessageView&)>;

/// @brief Builds the per-type expected wire-size table at compile time, mirroring
///        the eager parser's so the overlay validates frame lengths identically.
///
/// @return An array indexed by message-type byte, holding the expected wire size
///         for each known message type (zero for unknown types).
[[nodiscard]] consteval auto build_size_table() -> std::array<std::uint16_t, 256> {
    std::array<std::uint16_t, 256> table {};
    auto                           add = [&table]<typename MsgType>(char type) {
        table[static_cast<unsigned char>(type)] =
            static_cast<std::uint16_t>(itch::detail::WIRE_SIZE<MsgType>);
    };
    itch::detail::for_each_message_type(add);
    return table;
}

inline constexpr auto SIZE_TABLE = build_size_table();

/// @brief Frames a buffer and invokes `callback` with a zero-copy `MessageView`
///        for each well-formed message.
///
/// Framing and length validation match `Parser`: the 2-byte length prefix is
/// honoured, unknown type bytes and undersized frames are skipped. Unlike the
/// eager parser, no fields are decoded; the callback receives a view it can
/// inspect lazily.
///
/// @param data The raw buffer containing one or more length-prefixed ITCH frames.
/// @param callback Invoked with a `MessageView` for each well-formed frame found.
/// @return The number of views delivered to `callback`.
inline auto for_each_message(std::span<const std::byte> data, const ViewCallback& callback)
    -> std::uint64_t {
    std::uint64_t delivered = 0;
    std::size_t   offset    = 0;
    while (offset + sizeof(std::uint16_t) <= data.size()) {
        std::uint16_t length {};
        std::memcpy(&length, data.data() + offset, sizeof(length));
        length = utils::from_big_endian(length);
        offset += sizeof(std::uint16_t);
        if (length == 0) {
            continue;
        }
        if (offset + length > data.size()) {
            break;
        }
        const std::byte* frame        = data.data() + offset;
        const auto       message_type = static_cast<unsigned char>(frame[0]);
        offset += length;

        const std::uint16_t expected = SIZE_TABLE[message_type];
        if (expected == 0 || length < expected) {
            continue;  // Unknown type or undersized frame.
        }
        callback(MessageView {frame, length});
        ++delivered;
    }
    return delivered;
}

}  // namespace itch::overlay
