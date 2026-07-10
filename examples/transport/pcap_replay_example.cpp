#include <cstdint>
#include <format>
#include <iostream>
#include <string>

#include "itch/messages.hpp"
#include "itch/transport/pcap.hpp"

// Replays an ITCH feed straight from a captured .pcap/.pcapng file: the reader
// unwraps the Ethernet/IP/UDP and MoldUDP64 framing and yields decoded ITCH
// messages, with sequence gaps surfaced through the embedded tracker.
auto main(int argc, char* argv[]) -> int {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} <capture.pcap[ng]> [udp_port]\n", argv[0]);
        return 1;
    }

    std::uint64_t               message_count = 0;
    itch::transport::PcapReader reader {[&](const itch::Message& message) {
        ++message_count;
        const char type = std::visit([](const auto& msg) { return msg.message_type; }, message);
        if (message_count <= 10) {
            std::cout << std::format("message {:>3}: type '{}'\n", message_count, type);
        }
    }};

    reader.mold_decoder().tracker().set_gap_callback(
        [](std::string_view session, std::uint64_t expected, std::uint64_t received) {
            std::cerr << std::format(
                "gap on session '{}': expected {} but got {}\n", session, expected, received
            );
        }
    );

    if (argc >= 3) {
        reader.set_udp_port_filter(static_cast<std::uint16_t>(std::stoul(argv[2])));
    }

    if (!reader.read_file(argv[1])) {
        std::cerr << std::format("Error: '{}' is not a readable pcap/pcapng capture.\n", argv[1]);
        return 1;
    }

    std::cout << std::format(
        "Decoded {} ITCH messages from {} UDP datagrams ({} missing messages detected).\n",
        reader.messages_decoded(),
        reader.udp_datagrams(),
        reader.mold_decoder().tracker().gap_count()
    );
    return 0;
}
