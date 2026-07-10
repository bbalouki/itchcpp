#pragma once

/// @file
/// @brief Test-only fixture-building helpers that construct synthetic
///        ITCH/MoldUDP64/SoupBinTCP/pcap frames for unit tests.
///
/// These helpers build raw ITCH message payloads and wrap them in the
/// MoldUDP64, SoupBinTCP, and pcap envelopes entirely from in-memory bytes,
/// so the transport decoders can be exercised in tests without any real
/// capture files. This is a test fixture helper, not part of the public
/// library API.
///
/// @author Bertin Balouki SIMYELI

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "itch/parser.hpp"

namespace itch::test {

/// @brief Packs `value` into 2 bytes in big-endian order and appends them to
///        `out`.
/// @param out The byte buffer to append to.
/// @param value The 16-bit value to pack.
inline auto append_be16(std::vector<std::byte>& out, std::uint16_t value) -> void {
    const auto                         big = itch::utils::from_big_endian(value);
    std::array<std::byte, sizeof(big)> bytes {};
    std::memcpy(bytes.data(), &big, sizeof(big));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

/// @brief Packs `value` into 4 bytes in big-endian order and appends them to
///        `out`.
/// @param out The byte buffer to append to.
/// @param value The 32-bit value to pack.
inline auto append_be32(std::vector<std::byte>& out, std::uint32_t value) -> void {
    const auto                         big = itch::utils::from_big_endian(value);
    std::array<std::byte, sizeof(big)> bytes {};
    std::memcpy(bytes.data(), &big, sizeof(big));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

/// @brief Packs `value` into 8 bytes in big-endian order and appends them to
///        `out`.
/// @param out The byte buffer to append to.
/// @param value The 64-bit value to pack.
inline auto append_be64(std::vector<std::byte>& out, std::uint64_t value) -> void {
    const auto                         big = itch::utils::from_big_endian(value);
    std::array<std::byte, sizeof(big)> bytes {};
    std::memcpy(bytes.data(), &big, sizeof(big));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

/// @brief Packs `value` into 4 bytes in little-endian order (a direct copy of
///        the in-memory representation, assuming a little-endian host) and
///        appends them to `out`.
/// @param out The byte buffer to append to.
/// @param value The 32-bit value to pack.
inline auto append_le32(std::vector<std::byte>& out, std::uint32_t value) -> void {
    std::array<std::byte, sizeof(value)> bytes {};
    std::memcpy(bytes.data(), &value, sizeof(value));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

/// @brief Appends the raw contents of `src` to `out`.
/// @param out The byte buffer to append to.
/// @param src The bytes to append.
inline auto append_bytes(std::vector<std::byte>& out, const std::vector<std::byte>& src) -> void {
    out.insert(out.end(), src.begin(), src.end());
}

/// @brief Builds the raw payload (no length prefix) of a System Event message.
/// @param timestamp The 48-bit event timestamp (nanoseconds since midnight)
///        to encode.
/// @param event_code The single-character system event code.
/// @return The encoded System Event message payload.
inline auto system_event_payload(std::uint64_t timestamp, char event_code)
    -> std::vector<std::byte> {
    std::vector<std::byte> payload;
    payload.push_back(std::byte {'S'});
    append_be16(payload, 1);  // stock_locate
    append_be16(payload, 0);  // tracking_number
    // 48-bit timestamp: take the low 6 bytes of a big-endian 64-bit value.
    std::vector<std::byte> ts;
    append_be64(ts, timestamp);
    payload.insert(payload.end(), ts.begin() + 2, ts.end());
    payload.push_back(std::byte {static_cast<unsigned char>(event_code)});
    return payload;
}

/// @brief Builds the raw payload (no length prefix) of an Add Order (`A`)
///        message.
/// @param locate The stock locate code.
/// @param ref The order reference number.
/// @param side The single-character side code (`'B'` or `'S'`).
/// @param shares The number of shares in the order.
/// @param symbol The ticker symbol (right-padded/truncated to 8 characters).
/// @param price The raw (unscaled) limit price.
/// @return The encoded Add Order message payload.
inline auto add_order_payload(
    std::uint16_t    locate,
    std::uint64_t    ref,
    char             side,
    std::uint32_t    shares,
    std::string_view symbol,
    std::uint32_t    price
) -> std::vector<std::byte> {
    std::vector<std::byte> payload;
    payload.push_back(std::byte {'A'});
    append_be16(payload, locate);
    append_be16(payload, 0);  // tracking_number
    std::vector<std::byte> ts;
    append_be64(ts, 1234567);
    payload.insert(payload.end(), ts.begin() + 2, ts.end());  // 48-bit timestamp
    append_be64(payload, ref);
    payload.push_back(std::byte {static_cast<unsigned char>(side)});
    append_be32(payload, shares);
    for (std::size_t index = 0; index < 8; ++index) {
        const char chr = index < symbol.size() ? symbol[index] : ' ';
        payload.push_back(std::byte {static_cast<unsigned char>(chr)});
    }
    append_be32(payload, price);
    return payload;
}

/// @brief Wraps a raw payload as a length-prefixed ITCH frame (and message
///        block, since both use the same 2-byte big-endian length prefix).
/// @param payload The raw payload to prefix.
/// @return The payload prefixed with its 2-byte big-endian length.
inline auto length_prefixed(const std::vector<std::byte>& payload) -> std::vector<std::byte> {
    std::vector<std::byte> frame;
    append_be16(frame, static_cast<std::uint16_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

/// @brief Builds a complete MoldUDP64 datagram from a list of raw message
///        payloads.
/// @param session The session identifier (right-padded/truncated to 10
///        characters).
/// @param sequence The sequence number of the first message block.
/// @param payloads The raw message payloads to include, in order.
/// @return The encoded MoldUDP64 datagram.
inline auto moldudp64_packet(
    const std::string&                         session,
    std::uint64_t                              sequence,
    const std::vector<std::vector<std::byte>>& payloads
) -> std::vector<std::byte> {
    std::vector<std::byte> packet;
    std::array<char, 10>   session_field {};
    for (std::size_t index = 0; index < session_field.size(); ++index) {
        session_field[index] = index < session.size() ? session[index] : ' ';
    }
    for (char chr : session_field) {
        packet.push_back(std::byte {static_cast<unsigned char>(chr)});
    }
    append_be64(packet, sequence);
    append_be16(packet, static_cast<std::uint16_t>(payloads.size()));
    for (const auto& payload : payloads) {
        append_bytes(packet, length_prefixed(payload));
    }
    return packet;
}

/// @brief Builds a SoupBinTCP packet (2-byte length + type byte + payload).
/// @param type The single-character SoupBinTCP packet type.
/// @param payload The packet body following the type byte.
/// @return The encoded SoupBinTCP packet.
inline auto soupbin_packet(char type, const std::vector<std::byte>& payload)
    -> std::vector<std::byte> {
    std::vector<std::byte> packet;
    append_be16(packet, static_cast<std::uint16_t>(payload.size() + 1));
    packet.push_back(std::byte {static_cast<unsigned char>(type)});
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

/// @brief Wraps a UDP payload in Ethernet/IPv4/UDP headers (a single frame).
/// @param dst_port The destination UDP port to encode in the UDP header.
/// @param payload The UDP payload to wrap.
/// @return The encoded Ethernet/IPv4/UDP frame.
inline auto ethernet_ipv4_udp_frame(std::uint16_t dst_port, const std::vector<std::byte>& payload)
    -> std::vector<std::byte> {
    std::vector<std::byte> frame;
    // Ethernet header: dst(6), src(6), ethertype(2).
    for (int index = 0; index < 12; ++index) {
        frame.push_back(std::byte {0});
    }
    append_be16(frame, 0x0800);  // IPv4

    const std::uint16_t udp_len = static_cast<std::uint16_t>(8 + payload.size());
    const std::uint16_t ip_len  = static_cast<std::uint16_t>(20 + udp_len);

    // IPv4 header (20 bytes, no options).
    frame.push_back(std::byte {0x45});  // version 4, IHL 5
    frame.push_back(std::byte {0});     // DSCP/ECN
    append_be16(frame, ip_len);         // total length
    append_be16(frame, 0);              // identification
    append_be16(frame, 0);              // flags + fragment offset
    frame.push_back(std::byte {64});    // TTL
    frame.push_back(std::byte {17});    // protocol = UDP
    append_be16(frame, 0);              // header checksum (ignored)
    append_be32(frame, 0x7F000001);     // src ip
    append_be32(frame, 0x7F000001);     // dst ip

    // UDP header (8 bytes).
    append_be16(frame, 12345);     // src port
    append_be16(frame, dst_port);  // dst port
    append_be16(frame, udp_len);   // length
    append_be16(frame, 0);         // checksum (ignored)

    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

/// @brief Builds a classic little-endian .pcap file from a list of frames.
/// @param frames The captured link-layer frames to include, in order.
/// @return The encoded classic pcap file contents.
inline auto classic_pcap(const std::vector<std::vector<std::byte>>& frames
) -> std::vector<std::byte> {
    std::vector<std::byte> file;
    append_le32(file, 0xA1B2C3D4);  // magic
    file.push_back(std::byte {2});  // version major (LE)
    file.push_back(std::byte {0});
    file.push_back(std::byte {4});  // version minor
    file.push_back(std::byte {0});
    append_le32(file, 0);      // thiszone
    append_le32(file, 0);      // sigfigs
    append_le32(file, 65535);  // snaplen
    append_le32(file, 1);      // network = Ethernet

    for (const auto& frame : frames) {
        append_le32(file, 0);                                         // ts_sec
        append_le32(file, 0);                                         // ts_usec
        append_le32(file, static_cast<std::uint32_t>(frame.size()));  // incl_len
        append_le32(file, static_cast<std::uint32_t>(frame.size()));  // orig_len
        file.insert(file.end(), frame.begin(), frame.end());
    }
    return file;
}

}  // namespace itch::test
