#pragma once

#include <functional>
#include <istream>
#include <map>
#include <vector>

#include "messages.hpp"

namespace itch {

/**
 * @brief The signature for the callback function used in streaming parse
 * methods.
 *
 * @param const Message& A const reference to the fully parsed message object.
 */
using MessageCallback = std::function<void(const Message&)>;

/**
 * @brief A high-performance parser for the NASDAQ TotalView-ITCH 5.0 protocol.
 *
 * This class is designed to parse a raw binary feed of ITCH 5.0 messages,
 * handle message framing (based on the 2-byte length prefix), and deserialize
 * the byte payloads into structured, type-safe C++ objects.
 *
 * The primary interface is designed for maximum performance, operating directly
 * on a pre-loaded, contiguous block of memory (a `const char*` buffer). This
 * approach avoids all per-message memory allocations and stream I/O overhead,
 * allowing for throughput measured in gigabytes per second.
 *
 * For convenience, the parser also provides `std::istream` based wrappers.
 * These are easier to use for simple cases but are significantly slower, as
 * they must read the entire stream into an in-memory buffer before parsing.
 *
 * @note This parser assumes the input is a raw, sequenced ITCH 5.0 feed
 *       without any higher-level protocol framing (e.g., SoupBinTCP packet
 * headers).
 */
class Parser {
   public:
    /**
     * @brief Constructs a Parser instance.
     *
     * The constructor pre-populates an internal dispatch table (a map of
     * handlers) by registering a specific parsing function for each known ITCH
     * message type. This setup makes the parsing process a fast lookup
     * operation at runtime.
     */
    Parser();

    /**
     * @brief Parses messages from a memory buffer and invokes a callback for
     * each.
     *
     * This is the core, high-performance parsing method. It iterates through
     * the provided memory buffer, identifies each message, and invokes the
     * callback with the parsed object. This approach is highly efficient as it
     * performs no memory allocations in its main loop.
     *
     * @param data A pointer to the start of the memory buffer containing ITCH
     * data.
     * @param size The total size of the buffer in bytes.
     * @param callback A function to be called for each successfully parsed
     * message.
     * @throw std::runtime_error if the buffer ends unexpectedly in the middle
     * of a message.
     */
    void parse(const char* data, size_t size, const MessageCallback& callback);

    /**
     * @brief Parses all messages from a memory buffer and returns them in a
     * vector.
     *
     * A convenience method that parses the entire buffer and collects all
     * messages into a single vector.
     *
     * @param data A pointer to the start of the memory buffer.
     * @param size The total size of the buffer in bytes.
     * @return A std::vector<Message> containing all parsed messages.
     * @throw std::runtime_error on buffer parsing errors.
     * @note Be cautious with very large buffers, as this will load all parsed
     *       messages into memory, consuming significant RAM.
     */
    std::vector<Message> parse(const char* data, size_t size);

    /**
     * @brief Parses messages from a buffer, returning only those of specified
     * types.
     *
     * A convenience method that parses the entire buffer but only collects
     * messages whose type character is present in the `messages_to_keep` filter
     * list.
     *
     * @param data A pointer to the start of the memory buffer.
     * @param size The total size of the buffer in bytes.
     * @param messages_to_keep A vector of message type characters to keep
     * (e.g., {'A', 'P'}).
     * @return A std::vector<Message> containing only the filtered messages.
     * @throw std::runtime_error on buffer parsing errors.
     */
    std::vector<Message> parse(const char* data, size_t size, const std::vector<char>& messages);

    /**
     * @brief [Convenience Wrapper] Parses messages from a stream via a
     * callback.
     *
     * This method reads the *entire* stream into an internal memory buffer and
     * then calls the high-performance buffer-based parser. It is provided for
     * ease of use with stream-based APIs.
     *
     * @param is A reference to an std::istream opened in binary mode.
     * @param callback A function to be called for each successfully parsed
     * message.
     * @throw std::runtime_error on stream reading errors.
     * @note For maximum performance, load the data into memory yourself and use
     *       the `const char*` overload directly.
     */
    void parse(std::istream& is, const MessageCallback& callback);

    /**
     * @brief [Convenience Wrapper] Parses all messages from a stream into a
     * vector.
     *
     * This method reads the *entire* stream into memory before parsing.
     *
     * @param is A reference to an std::istream opened in binary mode.
     * @return A std::vector<Message> containing all parsed messages.
     * @throw std::runtime_error on stream reading errors.
     * @note Be cautious with large files, as this will load the entire raw file
     *       *and* all parsed messages into memory.
     */
    std::vector<Message> parse(std::istream& is);

    /**
     * @brief [Convenience Wrapper] Parses and filters messages from a stream.
     *
     * This method reads the *entire* stream into memory before parsing and
     * filtering.
     *
     * @param is A reference to an std::istream opened in binary mode.
     * @param messages A vector of message type characters to keep (e.g., {'A',
     * 'E', 'P'}).
     * @return A std::vector<Message> containing only the filtered messages.
     * @throw std::runtime_error on stream reading errors.
     */
    std::vector<Message> parse(std::istream& is, const std::vector<char>& messages);

   private:
    using Handler = std::function<Message(const char*)>;
    std::map<char, Handler> m_handlers;

    /**
     * @brief A template function to register a handler for a specific
     * message type.
     * @tparam T The C++ struct type representing the message (e.g.,
     * AddOrderMessage).
     * @param type The character identifier for this message type (e.g.,
     * 'A').
     */
    template <typename T>
    void register_handler(char type);
};

namespace utils {

// Generic byte-swapping function for any integral type.
template <typename T>
T swap_bytes(T value) {
    static_assert(std::is_integral<T>::value, "swap_bytes can only be used with integral types");
    union {
        T       val;
        uint8_t bytes[sizeof(T)];
    } src, dst;
    src.val = value;
    for (size_t i = 0; i < sizeof(T); ++i) {
        dst.bytes[i] = src.bytes[sizeof(T) - 1 - i];
    }
    return dst.val;
}

// Check the system's endianness at compile time (or runtime as a fallback).
// This determines if we need to swap bytes at all.
inline bool is_little_endian();

// Converts a value from big-endian (network order) to the host's native byte
// order. On little-endian systems (like x86/x64), it swaps the bytes. On
// big-endian systems, it does nothing.
template <typename T>
T from_big_endian(T value);

// Unpacks a value of type T from the buffer at the given offset, updating
// the offset accordingly. Handles endianness conversion for integral types.
template <typename T>
T unpack(const char* buffer, size_t& offset);

// Specialization for char arrays (strings)
inline void unpack_string(const char* buffer, size_t& offset, char* dest, size_t size);

// ITCH timestamps are 48-bit big-endian integers.
// 2 bytes for high part, 4 bytes for low part.
inline uint64_t unpack_timestamp(const char* buffer, size_t& offset);
}  // namespace utils

}  // namespace itch
