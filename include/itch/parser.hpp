#pragma once

/// @file
/// @brief Public parsing interface for NASDAQ TotalView-ITCH 5.0 feeds, plus
///        the low-level byte-unpacking utilities that back it.
///
/// `Parser` frames and decodes a raw ITCH byte stream into the `Message`
/// variant defined in itch/messages.hpp, offering both throwing and
/// non-throwing (`std::expected`-based) entry points over buffers, spans, and
/// streams. The `itch::utils` helpers underneath handle endianness conversion
/// and fixed-width field extraction, and are also reused by the encoder.
///
/// @author Bertin Balouki SIMYELI

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <optional>
#include <span>
#include <vector>

#ifdef __cpp_lib_expected
#include <expected>
#endif

#include "itch/messages.hpp"

namespace itch {

/// @brief The signature for the callback function used in streaming parse
/// methods.
///
/// @param const Message& A const reference to the fully parsed message object.
using MessageCallback = std::function<void(const Message&)>;

/// @brief Categories of recoverable problems encountered while framing a feed.
///
/// These are surfaced through the non-throwing `try_parse` entry points and the
/// optional error callback, so callers can react without relying on exceptions
/// or on the library writing to a global stream.
enum class ParseError {
    truncated,      ///< The buffer ended in the middle of a frame.
    unknown_type,   ///< The message type byte does not correspond to a known message.
    size_mismatch,  ///< The declared length is shorter than the message type requires.
};

/// @brief The signature for the optional diagnostics callback.
///
/// @param ParseError The category of problem that occurred.
/// @param char The offending message type byte (`'\0'` when not applicable,
///             e.g. for a truncated header).
using ErrorCallback = std::function<void(ParseError, char)>;

/// @brief A high-performance parser for the NASDAQ TotalView-ITCH 5.0 protocol.
///
/// This class is designed to parse a raw binary feed of ITCH 5.0 messages,
/// handle message framing (based on the 2-byte length prefix), and deserialize
/// the byte payloads into structured, type-safe C++ objects.
///
/// The primary interface is designed for maximum performance, operating directly
/// on a pre-loaded, contiguous block of memory (a `const char*` buffer). This
/// approach avoids all per-message memory allocations and stream I/O overhead,
/// allowing for throughput measured in gigabytes per second.
///
/// For convenience, the parser also provides `std::istream` based wrappers.
/// These are easier to use for simple cases but are significantly slower, as
/// they must read the entire stream into an in-memory buffer before parsing.
///
/// @note This parser assumes the input is a raw, sequenced ITCH 5.0 feed
///       without any higher-level protocol framing (e.g., SoupBinTCP packet
/// headers).
class Parser {
   public:
    /// @brief Constructs a Parser instance.
    ///
    /// Dispatch is driven by a compile-time table keyed on the message type
    /// byte, so no per-instance setup is required.
    Parser() = default;

    /// @brief Parses messages from a memory buffer and invokes a callback for
    /// each.
    ///
    /// This is the core, high-performance parsing method. It iterates through
    /// the provided memory buffer, identifies each message, and invokes the
    /// callback with the parsed object. This approach is highly efficient as it
    /// performs no memory allocations in its main loop.
    ///
    /// @param data A pointer to the start of the memory buffer containing ITCH
    /// data.
    /// @param size The total size of the buffer in bytes.
    /// @param callback A function to be called for each successfully parsed
    /// message.
    /// @throw std::runtime_error if the buffer ends unexpectedly in the middle
    /// of a message.
    auto parse(const char* data, size_t size, const MessageCallback& callback) -> void;

    /// @brief Parses all messages from a memory buffer and returns them in a
    /// vector.
    ///
    /// A convenience method that parses the entire buffer and collects all
    /// messages into a single vector.
    ///
    /// @param data A pointer to the start of the memory buffer.
    /// @param size The total size of the buffer in bytes.
    /// @return A std::vector<Message> containing all parsed messages.
    /// @throw std::runtime_error on buffer parsing errors.
    /// @note Be cautious with very large buffers, as this will load all parsed
    ///       messages into memory, consuming significant RAM.
    auto parse(const char* data, size_t size) -> std::vector<Message>;

    /// @brief Parses messages from a buffer, returning only those of specified
    /// types.
    ///
    /// A convenience method that parses the entire buffer but only collects
    /// messages whose type character is present in the `messages` filter
    /// list.
    ///
    /// @param data A pointer to the start of the memory buffer.
    /// @param size The total size of the buffer in bytes.
    /// @param messages A vector of message type characters to keep
    /// (e.g., {'A', 'P'}).
    /// @return A std::vector<Message> containing only the filtered messages.
    /// @throw std::runtime_error on buffer parsing errors.
    auto parse(const char* data, size_t size, const std::vector<char>& messages)
        -> std::vector<Message>;

    /// @brief [Convenience Wrapper] Parses messages from a stream via a
    /// callback.
    ///
    /// This method reads the *entire* stream into an internal memory buffer and
    /// then calls the high-performance buffer-based parser. It is provided for
    /// ease of use with stream-based APIs.
    ///
    /// @param data A reference to an std::istream opened in binary mode.
    /// @param callback A function to be called for each successfully parsed
    /// message.
    /// @throw std::runtime_error on stream reading errors.
    /// @note For maximum performance, load the data into memory yourself and use
    ///       the `const char*` overload directly.
    auto parse(std::istream& data, const MessageCallback& callback) -> void;

    /// @brief [Convenience Wrapper] Parses all messages from a stream into a
    /// vector.
    ///
    /// This method reads the *entire* stream into memory before parsing.
    ///
    /// @param data A reference to an std::istream opened in binary mode.
    /// @return A std::vector<Message> containing all parsed messages.
    /// @throw std::runtime_error on stream reading errors.
    /// @note Be cautious with large files, as this will load the entire raw file
    ///       *and* all parsed messages into memory.
    auto parse(std::istream& data) -> std::vector<Message>;

    /// @brief [Convenience Wrapper] Parses and filters messages from a stream.
    ///
    /// This method reads the *entire* stream into memory before parsing and
    /// filtering.
    ///
    /// @param data A reference to an std::istream opened in binary mode.
    /// @param messages A vector of message type characters to keep (e.g., {'A',
    /// 'E', 'P'}).
    /// @return A std::vector<Message> containing only the filtered messages.
    /// @throw std::runtime_error on stream reading errors.
    auto parse(std::istream& data, const std::vector<char>& messages) -> std::vector<Message>;

    /// @brief Parses messages from a byte span and invokes a callback for each.
    ///
    /// The `std::span` overloads are the preferred modern interface: the span
    /// carries its own size, which prevents the pointer/length desynchronization
    /// that the raw `(const char*, size_t)` overloads are prone to. Those raw
    /// overloads are retained as thin shims for C interop.
    ///
    /// @param data A view over the contiguous buffer containing ITCH data.
    /// @param callback A function to be called for each successfully parsed
    /// message.
    /// @throw std::runtime_error if the buffer ends in the middle of a message.
    auto parse(std::span<const std::byte> data, const MessageCallback& callback) -> void;

    /// @brief Parses all messages from a byte span and returns them in a vector.
    ///
    /// @param data A view over the contiguous buffer containing ITCH data.
    /// @return A std::vector<Message> containing all parsed messages.
    /// @throw std::runtime_error if the buffer ends in the middle of a message.
    auto parse(std::span<const std::byte> data) -> std::vector<Message>;

    /// @brief Parses messages from a byte span, keeping only the requested types.
    ///
    /// @param data A view over the contiguous buffer containing ITCH data.
    /// @param messages A vector of message type characters to keep (e.g., {'A', 'P'}).
    /// @return A std::vector<Message> containing only the filtered messages.
    /// @throw std::runtime_error if the buffer ends in the middle of a message.
    auto parse(std::span<const std::byte> data, const std::vector<char>& messages)
        -> std::vector<Message>;

#ifdef __cpp_lib_expected
    /// @brief Non-throwing parse: invokes a callback per message, reporting
    ///        truncation through the return value instead of an exception.
    ///
    /// This is the latency-friendly entry point for callers that prefer not to
    /// pay for exceptions on the hot path. Unknown message types and oversized
    /// or undersized frames are routed to the diagnostics policy (counters plus
    /// the optional error callback) and skipped; only an unrecoverable
    /// truncation yields an `unexpected` result.
    ///
    /// Available when the standard library provides `std::expected` (C++23).
    ///
    /// @param data A view over the contiguous buffer containing ITCH data.
    /// @param callback A function to be called for each successfully parsed
    /// message.
    /// @return Nothing on success, or a `ParseError` describing the failure.
    [[nodiscard]] auto try_parse(std::span<const std::byte> data, const MessageCallback& callback)
        -> std::expected<void, ParseError>;

    /// @brief Non-throwing parse that collects all messages into a vector.
    ///
    /// @param data A view over the contiguous buffer containing ITCH data.
    /// @return All parsed messages on success, or a `ParseError` describing the failure.
    [[nodiscard]] auto try_parse(std::span<const std::byte> data)
        -> std::expected<std::vector<Message>, ParseError>;
#endif

    /// @brief Registers a callback invoked for each recoverable framing problem.
    ///
    /// Passing an empty function clears any previously installed callback. The
    /// default behavior, with no callback installed, is to silently skip and
    /// count the offending frame.
    ///
    /// @param callback The diagnostics callback to install, or an empty function
    ///                 to clear any previously installed callback.
    auto set_error_callback(ErrorCallback callback) -> void;

    /// @brief The number of frames skipped because their type byte was unknown.
    ///
    /// @return The running count of frames skipped for an unrecognized type byte.
    [[nodiscard]] auto unknown_message_count() const noexcept -> std::uint64_t {
        return m_unknown_message_count;
    }

    /// @brief The number of frames skipped because their declared length was too
    ///        small for the message type.
    ///
    /// @return The running count of frames skipped for a declared length too
    ///         small for their message type.
    [[nodiscard]] auto malformed_message_count() const noexcept -> std::uint64_t {
        return m_malformed_message_count;
    }

    /// @brief Resets the accumulating diagnostics counters to zero.
    auto reset_diagnostics() noexcept -> void {
        m_unknown_message_count   = 0;
        m_malformed_message_count = 0;
    }

   private:
    /// @brief The shared, non-throwing framing loop backing every parse overload.
    ///
    /// @param data A pointer to the start of the memory buffer containing ITCH data.
    /// @param size The total size of the buffer in bytes.
    /// @param callback A function to be called for each successfully parsed message.
    /// @return `std::nullopt` on success, or the `ParseError` that aborted the
    ///         loop (only an unrecoverable truncation aborts it).
    auto parse_impl(const char* data, std::size_t size, const MessageCallback& callback)
        -> std::optional<ParseError>;

    /// @brief Records a recoverable framing problem and notifies the callback.
    ///
    /// @param error The category of problem that occurred.
    /// @param message_type The offending message type byte (`'\0'` when not
    ///                     applicable, e.g. for a truncated header).
    auto report_error(ParseError error, char message_type) -> void;

    ErrorCallback m_error_callback {};
    std::uint64_t m_unknown_message_count {0};
    std::uint64_t m_malformed_message_count {0};
};

namespace utils {

/// @brief Reverses the byte order of an integral value.
///
/// Prefers `std::byteswap` (C++23) and otherwise falls back to a well-defined
/// `std::bit_cast` based reversal. This deliberately avoids reading an inactive
/// union member, which is undefined behavior in C++ and a hazard on the parser
/// hot path.
///
/// @tparam IntType The integral type to byte-swap.
/// @param value The value whose byte order should be reversed.
/// @return `value` with its byte order reversed.
template <std::integral IntType>
[[nodiscard]] constexpr auto swap_bytes(IntType value) noexcept -> IntType {
#ifdef __cpp_lib_byteswap
    return std::byteswap(value);
#else
    if constexpr (sizeof(IntType) == 1) {
        return value;
    } else {
        auto bytes = std::bit_cast<std::array<std::byte, sizeof(IntType)>>(value);
        std::ranges::reverse(bytes);
        return std::bit_cast<IntType>(bytes);
    }
#endif
}

/// @brief Converts a big-endian (network order) value to the host byte order.
///
/// On little-endian hosts this swaps the bytes; on big-endian hosts it is a
/// no-op. The choice is made at compile time via `std::endian`.
///
/// @tparam IntType The integral type to convert.
/// @param value The big-endian value to convert.
/// @return `value` converted to host byte order.
template <std::integral IntType>
[[nodiscard]] constexpr auto from_big_endian(IntType value) noexcept -> IntType {
    if constexpr (std::endian::native == std::endian::little) {
        return swap_bytes(value);
    } else {
        return value;
    }
}

/// @brief Unpacks a value of type T from the buffer, advancing the offset.
///        Handles endianness conversion for multi-byte integral types.
///
/// @tparam ValueType The type of value to unpack.
/// @param buffer The buffer to read from.
/// @param offset The byte offset to read from, advanced past the unpacked value.
/// @return The unpacked value, converted to host byte order when applicable.
template <typename ValueType>
auto unpack(const char* buffer, std::size_t& offset) -> ValueType;

/// @brief Copies a fixed-width character field out of the buffer.
///
/// @param buffer The buffer to read from.
/// @param offset The byte offset to read from, advanced past the copied field.
/// @param dest The destination buffer to copy the field into.
/// @param size The number of characters to copy.
inline auto unpack_string(const char* buffer, std::size_t& offset, char* dest, std::size_t size)
    -> void;

/// @brief Unpacks a 48-bit big-endian ITCH timestamp (2-byte high, 4-byte low).
///
/// @param buffer The buffer to read from.
/// @param offset The byte offset to read from, advanced past the timestamp.
/// @return The timestamp as nanoseconds past midnight.
inline auto unpack_timestamp(const char* buffer, std::size_t& offset) -> std::uint64_t;
}  // namespace utils

}  // namespace itch
