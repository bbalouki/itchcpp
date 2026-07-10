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
#include "itch/transport/moldudp64.hpp"
#include "itch/transport/sequencing.hpp"

namespace {

constexpr int         BYTE_BITS    = 8;
constexpr unsigned    BYTE_MASK    = 0xFF;
constexpr std::size_t SESSION_SIZE = 10;

// Appends `width` bytes of `value` to `buffer`, most significant byte first.
auto append_big_endian(std::vector<std::byte>& buffer, std::uint64_t value, std::size_t width)
    -> void {
    for (std::size_t index = 0; index < width; ++index) {
        const std::size_t shift = (width - 1 - index) * static_cast<std::size_t>(BYTE_BITS);
        buffer.push_back(static_cast<std::byte>((value >> shift) & BYTE_MASK));
    }
}

// Builds one MoldUDP64 datagram: the 20-byte header followed by one
// length-prefixed message block (itch::encode_frame) per message.
auto make_packet(
    std::string_view                  session,
    std::uint64_t                     sequence_number,
    const std::vector<itch::Message>& messages
) -> std::vector<std::byte> {
    std::string padded_session {session};
    padded_session.resize(SESSION_SIZE, ' ');

    std::vector<std::byte> packet;
    for (const char letter : padded_session) {
        packet.push_back(static_cast<std::byte>(letter));
    }
    append_big_endian(packet, sequence_number, sizeof(std::uint64_t));
    append_big_endian(packet, messages.size(), sizeof(std::uint16_t));
    for (const auto& message : messages) {
        const auto frame = itch::encode_frame(message);
        packet.insert(packet.end(), frame.begin(), frame.end());
    }
    return packet;
}

// A minimal RetransmitRequester that only prints what it would ask for; a real
// implementation would issue a MoldUDP64 rewind/retransmit request here and
// feed the recovered messages back through decode_packet().
class PrintingRetransmitRequester : public itch::transport::RetransmitRequester {
   public:
    auto request_retransmit(
        std::string_view session, std::uint64_t start_sequence, std::uint64_t count
    ) -> void override {
        std::cout << std::format(
            "retransmit request: session '{}', start {}, count {}\n", session, start_sequence, count
        );
    }
};

}  // namespace

// Demonstrates decoding MoldUDP64 datagrams directly (no pcap capture
// involved): a normal packet followed by one that skips two sequence numbers,
// showing the embedded SequenceTracker detect the gap and drive a
// RetransmitRequester recovery hook.
auto main() -> int {
    std::uint64_t                     decoded_count = 0;
    itch::transport::MoldUdp64Decoder decoder {[&](const itch::Message& message) {
        ++decoded_count;
        const char type = std::visit([](const auto& msg) { return msg.message_type; }, message);
        std::cout << std::format("decoded message {}: type '{}'\n", decoded_count, type);
    }};

    const itch::Message first_order = itch::AddOrderMessage {
        .stock_locate           = 1,
        .tracking_number        = 0,
        .timestamp              = 34200000000000ULL,
        .order_reference_number = 2001,
        .buy_sell_indicator     = 'B',
        .shares                 = 300,
        .stock                  = {'G', 'O', 'O', 'G', ' ', ' ', ' ', ' '},
        .price                  = 2750000,
    };
    const itch::Message second_order = itch::AddOrderMessage {
        .stock_locate           = 1,
        .tracking_number        = 0,
        .timestamp              = 34200100000000ULL,
        .order_reference_number = 2002,
        .buy_sell_indicator     = 'S',
        .shares                 = 150,
        .stock                  = {'G', 'O', 'O', 'G', ' ', ' ', ' ', ' '},
        .price                  = 2751000,
    };
    const itch::Message third_order = itch::AddOrderMessage {
        .stock_locate           = 1,
        .tracking_number        = 0,
        .timestamp              = 34200200000000ULL,
        .order_reference_number = 2003,
        .buy_sell_indicator     = 'B',
        .shares                 = 75,
        .stock                  = {'G', 'O', 'O', 'G', ' ', ' ', ' ', ' '},
        .price                  = 2749000,
    };

    const auto first_packet = make_packet("DEMO", 1, {first_order, second_order});
    decoder.decode_packet(first_packet);

    // Install the recovery hooks before feeding the gapped packet: sequence 1
    // carried 2 messages, so 3 is expected next, but the next packet starts at
    // 5, leaving sequences 3 and 4 missing.
    decoder.tracker().set_gap_callback(
        [](std::string_view session, std::uint64_t expected, std::uint64_t received) {
            std::cerr << std::format(
                "gap on session '{}': expected {} but got {}\n", session, expected, received
            );
        }
    );
    PrintingRetransmitRequester requester;
    decoder.tracker().set_retransmit_requester(&requester);

    const auto second_packet = make_packet("DEMO", 5, {third_order});
    decoder.decode_packet(second_packet);

    std::cout << std::format(
        "{} packets decoded, {} messages decoded, {} gaps detected\n",
        decoder.packets_decoded(),
        decoder.messages_decoded(),
        decoder.tracker().gap_count()
    );
    return 0;
}
