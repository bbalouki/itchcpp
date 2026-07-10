#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/transport/soupbintcp.hpp"

namespace {

constexpr int         BYTE_BITS     = 8;
constexpr unsigned    BYTE_MASK     = 0xFF;
constexpr std::size_t SESSION_SIZE  = 10;
constexpr std::size_t SEQUENCE_SIZE = 20;

// Appends a complete SoupBinTCP packet (2-byte big-endian length, 1-byte type,
// payload) to the byte stream under construction.
auto append_packet(
    std::vector<std::byte>&            stream,
    itch::transport::SoupBinPacketType type,
    std::span<const std::byte>         payload
) -> void {
    const auto length = static_cast<std::uint16_t>(payload.size() + 1);
    stream.push_back(static_cast<std::byte>((length >> BYTE_BITS) & BYTE_MASK));
    stream.push_back(static_cast<std::byte>(length & BYTE_MASK));
    stream.push_back(static_cast<std::byte>(type));
    stream.insert(stream.end(), payload.begin(), payload.end());
}

// Builds the Login Accepted payload: a space-padded session id followed by an
// ASCII, space-padded starting sequence number, per the SoupBinTCP spec.
auto make_login_accepted_payload(std::string_view session, std::uint64_t sequence)
    -> std::vector<std::byte> {
    std::string text {session};
    text.resize(SESSION_SIZE, ' ');
    std::string sequence_text = std::format("{}", sequence);
    sequence_text.resize(SEQUENCE_SIZE, ' ');
    text += sequence_text;

    std::vector<std::byte> payload(text.size());
    for (std::size_t index = 0; index < text.size(); ++index) {
        payload[index] = static_cast<std::byte>(text[index]);
    }
    return payload;
}

}  // namespace

// Demonstrates decoding a SoupBinTCP byte stream directly: a Login Accepted
// control packet followed by sequenced ITCH data packets, with one packet
// deliberately split across two feed() calls to show the decoder reassembling
// a packet that TCP delivered across multiple reads.
auto main() -> int {
    std::uint64_t                   decoded_count = 0;
    itch::transport::SoupBinDecoder decoder {[&](const itch::Message& message) {
        ++decoded_count;
        const char type = std::visit([](const auto& msg) { return msg.message_type; }, message);
        std::cout << std::format("decoded message {}: type '{}'\n", decoded_count, type);
    }};
    decoder.set_event_callback([](itch::transport::SoupBinPacketType type,
                                  std::span<const std::byte>         payload) {
        std::cout << std::format(
            "control packet: type '{}' ({} bytes)\n", static_cast<char>(type), payload.size()
        );
    });

    const itch::Message add_order = itch::AddOrderMessage {
        .stock_locate           = 1,
        .tracking_number        = 0,
        .timestamp              = 34200000000000ULL,
        .order_reference_number = 1001,
        .buy_sell_indicator     = 'B',
        .shares                 = 100,
        .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .price                  = 1500000,
    };
    const itch::Message executed = itch::OrderExecutedMessage {
        .stock_locate           = 1,
        .tracking_number        = 0,
        .timestamp              = 34200500000000ULL,
        .order_reference_number = 1001,
        .executed_shares        = 100,
        .match_number           = 5001,
    };
    const itch::Message second_order = itch::AddOrderMessage {
        .stock_locate           = 2,
        .tracking_number        = 0,
        .timestamp              = 34201000000000ULL,
        .order_reference_number = 1002,
        .buy_sell_indicator     = 'S',
        .shares                 = 200,
        .stock                  = {'M', 'S', 'F', 'T', ' ', ' ', ' ', ' '},
        .price                  = 3200000,
    };

    std::vector<std::byte> stream;
    append_packet(
        stream,
        itch::transport::SoupBinPacketType::login_accepted,
        make_login_accepted_payload("DEMO", 1)
    );
    append_packet(
        stream, itch::transport::SoupBinPacketType::sequenced_data, itch::encode_message(add_order)
    );
    append_packet(
        stream, itch::transport::SoupBinPacketType::sequenced_data, itch::encode_message(executed)
    );
    append_packet(
        stream,
        itch::transport::SoupBinPacketType::sequenced_data,
        itch::encode_message(second_order)
    );

    // Feed the stream in two chunks, splitting mid-packet, to prove feed()
    // reassembles a packet spanning multiple TCP reads rather than requiring
    // one call per packet.
    const std::size_t split = stream.size() / 2;
    decoder.feed(std::span {stream}.subspan(0, split));
    decoder.feed(std::span {stream}.subspan(split));

    std::cout << std::format(
        "session '{}', next sequence {}, {} messages decoded\n",
        decoder.current_session(),
        decoder.next_sequence(),
        decoder.messages_decoded()
    );
    return 0;
}
