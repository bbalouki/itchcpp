#include "itch/transport/soupbintcp.hpp"

#include <array>
#include <bit>
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace itch::transport {

namespace {

constexpr std::size_t LENGTH_PREFIX_SIZE   = 2;
constexpr std::size_t LOGIN_SESSION_SIZE   = 10;
constexpr std::size_t LOGIN_SEQUENCE_SIZE  = 20;
constexpr std::size_t MAX_WIRE_MESSAGE_LEN = 0xFFFF;

// Trims trailing spaces from a fixed-width ASCII field.
auto trim_field(std::span<const std::byte> field) -> std::string {
    std::size_t length = field.size();
    while (length > 0 && static_cast<char>(field[length - 1]) == ' ') {
        --length;
    }
    std::string result(length, '\0');
    for (std::size_t index = 0; index < length; ++index) {
        result[index] = static_cast<char>(field[index]);
    }
    return result;
}

// Parses the right-justified numeric sequence number field of Login Accepted.
auto parse_sequence_field(std::span<const std::byte> field) -> std::uint64_t {
    const std::string text  = trim_field(field);
    std::uint64_t     value = 0;
    const char*       begin = text.data();
    // Skip any leading spaces left after trimming only trailing ones.
    while (begin != text.data() + text.size() && *begin == ' ') {
        ++begin;
    }
    std::from_chars(begin, text.data() + text.size(), value);
    return value;
}

}  // namespace

SoupBinDecoder::SoupBinDecoder(MessageCallback callback) : m_callback {std::move(callback)} {}

auto SoupBinDecoder::set_event_callback(EventCallback callback) -> void {
    m_event_callback = std::move(callback);
}

auto SoupBinDecoder::feed(std::span<const std::byte> bytes) -> void {
    m_buffer.insert(m_buffer.end(), bytes.begin(), bytes.end());

    std::size_t offset = 0;
    while (m_buffer.size() - offset >= LENGTH_PREFIX_SIZE) {
        std::uint16_t packet_length {};
        std::memcpy(&packet_length, m_buffer.data() + offset, LENGTH_PREFIX_SIZE);
        packet_length = utils::from_big_endian(packet_length);

        if (packet_length == 0) {
            offset += LENGTH_PREFIX_SIZE;  // Defensive: skip a zero-length frame.
            continue;
        }
        if (m_buffer.size() - offset < LENGTH_PREFIX_SIZE + packet_length) {
            break;  // The rest of this packet has not arrived yet.
        }

        const auto type =
            static_cast<SoupBinPacketType>(static_cast<char>(m_buffer[offset + LENGTH_PREFIX_SIZE])
            );
        const std::span<const std::byte> payload {
            m_buffer.data() + offset + LENGTH_PREFIX_SIZE + 1,
            static_cast<std::size_t>(packet_length) - 1
        };
        process_packet(type, payload);
        offset += LENGTH_PREFIX_SIZE + packet_length;
    }

    // Drop the bytes we have fully consumed, keeping any partial trailing packet.
    if (offset > 0) {
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<std::ptrdiff_t>(offset));
    }
}

auto SoupBinDecoder::process_packet(SoupBinPacketType type, std::span<const std::byte> payload)
    -> void {
    switch (type) {
        case SoupBinPacketType::login_accepted: {
            if (payload.size() >= LOGIN_SESSION_SIZE + LOGIN_SEQUENCE_SIZE) {
                m_session = trim_field(payload.subspan(0, LOGIN_SESSION_SIZE));
                m_next_sequence =
                    parse_sequence_field(payload.subspan(LOGIN_SESSION_SIZE, LOGIN_SEQUENCE_SIZE));
                if (m_next_sequence == 0) {
                    m_next_sequence = 1;
                }
            }
            break;
        }
        case SoupBinPacketType::sequenced_data: {
            m_tracker.observe(m_session, m_next_sequence, 1);
            ++m_next_sequence;
            decode_application_message(payload);
            return;
        }
        case SoupBinPacketType::unsequenced_data: {
            decode_application_message(payload);
            return;
        }
        case SoupBinPacketType::debug:
        case SoupBinPacketType::login_rejected:
        case SoupBinPacketType::server_heartbeat:
        case SoupBinPacketType::end_of_session:
        case SoupBinPacketType::login_request:
        case SoupBinPacketType::client_heartbeat:
        case SoupBinPacketType::logout_request:
            break;
    }

    if (m_event_callback) {
        m_event_callback(type, payload);
    }
}

auto SoupBinDecoder::decode_application_message(std::span<const std::byte> payload) -> void {
    if (payload.empty() || payload.size() > MAX_WIRE_MESSAGE_LEN) {
        return;
    }

    // A data packet carries one ITCH message without its own 2-byte length
    // prefix; re-prefixing it lets the ordinary framing parser decode it. The
    // length is written big-endian (network order) to match the wire framing.
    std::vector<std::byte> framed;
    framed.reserve(LENGTH_PREFIX_SIZE + payload.size());
    const auto length     = static_cast<std::uint16_t>(payload.size());
    const auto big_endian = utils::from_big_endian(length);
    const auto len_bytes  = std::bit_cast<std::array<std::byte, LENGTH_PREFIX_SIZE>>(big_endian);
    framed.push_back(len_bytes[0]);
    framed.push_back(len_bytes[1]);
    framed.insert(framed.end(), payload.begin(), payload.end());

    auto counting_callback = [this](const Message& message) {
        ++m_messages_decoded;
        if (m_callback) {
            m_callback(message);
        }
    };
    // A malformed single-message payload is skipped rather than aborting the
    // whole stream. try_parse would avoid the exception, but it needs C++23's
    // std::expected and this decoder must also build under C++20.
    try {
        m_parser.parse(std::span<const std::byte> {framed}, counting_callback);
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const std::runtime_error&) {
    }
}

}  // namespace itch::transport
