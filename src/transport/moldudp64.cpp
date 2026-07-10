#include "itch/transport/moldudp64.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace itch::transport {

namespace {

// Reads a big-endian unsigned integer of the field's width out of a byte span.
template <typename UintType>
auto read_big_endian(std::span<const std::byte> data, std::size_t offset) -> UintType {
    UintType value {};
    std::memcpy(&value, data.data() + offset, sizeof(UintType));
    return utils::from_big_endian(value);
}

}  // namespace

MoldUdp64Decoder::MoldUdp64Decoder(MessageCallback callback) : m_callback {std::move(callback)} {}

auto MoldUdp64Decoder::decode_packet(std::span<const std::byte> packet
) -> std::optional<MoldUdp64Header> {
    if (packet.size() < HEADER_SIZE) {
        return std::nullopt;
    }

    ++m_packets_decoded;

    MoldUdp64Header header {};
    std::memcpy(header.session.data(), packet.data(), header.session.size());
    constexpr std::size_t SEQUENCE_OFFSET = 10;
    constexpr std::size_t COUNT_OFFSET    = 18;
    header.sequence_number                = read_big_endian<std::uint64_t>(packet, SEQUENCE_OFFSET);
    header.message_count                  = read_big_endian<std::uint16_t>(packet, COUNT_OFFSET);

    if (header.is_end_of_session()) {
        return header;  // Control packet: no message blocks, no sequence advance.
    }

    // The sequence number names the first message in the packet; feed the count
    // to the tracker so gaps in the multicast stream are detected. A heartbeat
    // (count 0) still anchors the next expected sequence.
    m_tracker.observe(header.session_view(), header.sequence_number, header.message_count);

    if (header.is_heartbeat()) {
        return header;
    }

    // After the header the datagram is a sequence of length-prefixed message
    // blocks, which is exactly the framing the ordinary parser consumes. A
    // malformed or truncated datagram is tolerated: the parser stops at the end
    // of the buffer and we report through the callback what was decoded.
    const std::span<const std::byte> blocks            = packet.subspan(HEADER_SIZE);
    auto                             counting_callback = [this](const Message& message) {
        ++m_messages_decoded;
        if (m_callback) {
            m_callback(message);
        }
    };
    try {
        m_parser.parse(blocks, counting_callback);
    } catch (const std::runtime_error&) {
        // A truncated trailing block in a single datagram is non-fatal here; the
        // gap will already have been surfaced by the sequence tracker.
    }

    return header;
}

}  // namespace itch::transport
