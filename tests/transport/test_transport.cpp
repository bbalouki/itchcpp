#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "itch/messages.hpp"
#include "itch/transport/moldudp64.hpp"
#include "itch/transport/pcap.hpp"
#include "itch/transport/sequencing.hpp"
#include "itch/transport/soupbintcp.hpp"
#include "transport/frame_builders.hpp"

namespace {

using itch::Message;
using itch::SystemEventMessage;

// Pulls the event_code out of the System Event messages decoded into a vector.
auto event_codes(const std::vector<Message>& messages) -> std::string {
    std::string codes;
    for (const auto& message : messages) {
        if (const auto* event = std::get_if<SystemEventMessage>(&message)) {
            codes.push_back(event->event_code);
        }
    }
    return codes;
}

}  // namespace

TEST(MoldUdp64, DecodesAllMessageBlocksInPacket) {
    std::vector<Message>              decoded;
    itch::transport::MoldUdp64Decoder decoder {[&](const Message& msg) { decoded.push_back(msg); }};

    const auto packet = itch::test::moldudp64_packet(
        "SESSION01",
        1,
        {itch::test::system_event_payload(1000, 'O'), itch::test::system_event_payload(2000, 'S')}
    );

    const auto header = decoder.decode_packet(std::span<const std::byte> {packet});
    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(header->session_view(), "SESSION01");
    EXPECT_EQ(header->sequence_number, 1U);
    EXPECT_EQ(header->message_count, 2U);
    EXPECT_EQ(decoder.messages_decoded(), 2U);
    EXPECT_EQ(event_codes(decoded), "OS");
}

TEST(MoldUdp64, HeartbeatCarriesNoMessages) {
    itch::transport::MoldUdp64Decoder decoder {[](const Message&) {}};
    const auto                        packet = itch::test::moldudp64_packet("SESSION01", 1, {});
    const auto header = decoder.decode_packet(std::span<const std::byte> {packet});
    ASSERT_TRUE(header.has_value());
    EXPECT_TRUE(header->is_heartbeat());
    EXPECT_EQ(decoder.messages_decoded(), 0U);
}

TEST(MoldUdp64, DetectsSequenceGapAcrossPackets) {
    itch::transport::MoldUdp64Decoder decoder {[](const Message&) {}};
    std::uint64_t                     reported_expected = 0;
    std::uint64_t                     reported_received = 0;
    decoder.tracker().set_gap_callback(
        [&](std::string_view, std::uint64_t expected, std::uint64_t received) {
            reported_expected = expected;
            reported_received = received;
        }
    );

    // Sequences 1 and 2, then jump to 5 (missing 3 and 4).
    const auto first = itch::test::moldudp64_packet(
        "S", 1, {itch::test::system_event_payload(1, 'O'), itch::test::system_event_payload(2, 'S')}
    );
    const auto second =
        itch::test::moldudp64_packet("S", 5, {itch::test::system_event_payload(3, 'Q')});
    decoder.decode_packet(std::span<const std::byte> {first});
    decoder.decode_packet(std::span<const std::byte> {second});

    EXPECT_EQ(decoder.tracker().gap_count(), 2U);
    EXPECT_EQ(reported_expected, 3U);
    EXPECT_EQ(reported_received, 5U);
}

TEST(MoldUdp64, ShortDatagramReturnsNullopt) {
    itch::transport::MoldUdp64Decoder decoder {[](const Message&) {}};
    std::vector<std::byte>            tiny(4, std::byte {0});
    EXPECT_FALSE(decoder.decode_packet(std::span<const std::byte> {tiny}).has_value());
}

TEST(SoupBinTcp, DecodesSequencedDataAcrossSegmentBoundaries) {
    std::vector<Message>            decoded;
    itch::transport::SoupBinDecoder decoder {[&](const Message& msg) { decoded.push_back(msg); }};

    const auto packet1 = itch::test::soupbin_packet('S', itch::test::system_event_payload(10, 'O'));
    const auto packet2 = itch::test::soupbin_packet('S', itch::test::system_event_payload(20, 'Q'));

    // Concatenate and feed in two chunks that split a packet in half.
    std::vector<std::byte> stream;
    stream.insert(stream.end(), packet1.begin(), packet1.end());
    stream.insert(stream.end(), packet2.begin(), packet2.end());
    const std::size_t split = packet1.size() + 2;
    decoder.feed(std::span<const std::byte> {stream.data(), split});
    EXPECT_EQ(decoded.size(), 1U);  // Second packet is still incomplete.
    decoder.feed(std::span<const std::byte> {stream.data() + split, stream.size() - split});

    EXPECT_EQ(decoder.messages_decoded(), 2U);
    EXPECT_EQ(event_codes(decoded), "OQ");
}

TEST(SoupBinTcp, LoginAcceptedSetsSessionAndSequence) {
    itch::transport::SoupBinDecoder decoder {[](const Message&) {}};

    std::vector<std::byte> login;
    const std::string      session = "GLIMPSE   ";  // 10 chars
    for (char chr : session) {
        login.push_back(std::byte {static_cast<unsigned char>(chr)});
    }
    const std::string sequence = "                  42";  // 20 chars, right-justified
    for (char chr : sequence) {
        login.push_back(std::byte {static_cast<unsigned char>(chr)});
    }
    const auto packet = itch::test::soupbin_packet('A', login);
    decoder.feed(std::span<const std::byte> {packet});

    EXPECT_EQ(decoder.current_session(), "GLIMPSE");
    EXPECT_EQ(decoder.next_sequence(), 42U);
}

TEST(SoupBinTcp, ControlPacketsReachEventCallback) {
    itch::transport::SoupBinDecoder decoder {[](const Message&) {}};
    char                            seen = '\0';
    decoder.set_event_callback([&](itch::transport::SoupBinPacketType type,
                                   std::span<const std::byte>) { seen = static_cast<char>(type); });
    const auto heartbeat = itch::test::soupbin_packet('H', {});
    decoder.feed(std::span<const std::byte> {heartbeat});
    EXPECT_EQ(seen, 'H');
}

TEST(Pcap, ReplaysMessagesFromClassicCapture) {
    std::vector<Message>        decoded;
    itch::transport::PcapReader reader {[&](const Message& msg) { decoded.push_back(msg); }};

    const auto mold = itch::test::moldudp64_packet(
        "SESSION01",
        1,
        {itch::test::system_event_payload(1, 'O'), itch::test::system_event_payload(2, 'C')}
    );
    const auto frame = itch::test::ethernet_ipv4_udp_frame(26477, mold);
    const auto file  = itch::test::classic_pcap({frame});

    EXPECT_TRUE(reader.read(std::span<const std::byte> {file}));
    EXPECT_EQ(reader.udp_datagrams(), 1U);
    EXPECT_EQ(reader.messages_decoded(), 2U);
    EXPECT_EQ(event_codes(decoded), "OC");
}

TEST(Pcap, PortFilterDropsNonMatchingDatagrams) {
    std::vector<Message>        decoded;
    itch::transport::PcapReader reader {[&](const Message& msg) { decoded.push_back(msg); }};
    reader.set_udp_port_filter(9999);

    const auto mold =
        itch::test::moldudp64_packet("S", 1, {itch::test::system_event_payload(1, 'O')});
    const auto frame = itch::test::ethernet_ipv4_udp_frame(26477, mold);
    const auto file  = itch::test::classic_pcap({frame});

    EXPECT_TRUE(reader.read(std::span<const std::byte> {file}));
    EXPECT_EQ(reader.messages_decoded(), 0U);
}

TEST(Pcap, UnrecognizedBufferIsRejected) {
    itch::transport::PcapReader reader {[](const Message&) {}};
    std::vector<std::byte>      junk(64, std::byte {0x11});
    EXPECT_FALSE(reader.read(std::span<const std::byte> {junk}));
}

TEST(SequenceTracker, IgnoresDuplicateReplayedMessages) {
    itch::transport::SequenceTracker tracker;
    EXPECT_EQ(tracker.observe("S", 1, 3), 0U);  // messages 1,2,3
    EXPECT_EQ(tracker.observe("S", 2, 3), 0U);  // overlapping replay 2,3,4
    EXPECT_EQ(tracker.messages_seen(), 4U);     // only 1,2,3,4 counted once
    EXPECT_EQ(tracker.expected_next("S"), 5U);
}
