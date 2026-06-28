#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "itch/parser.hpp"
#include "itch/transport/moldudp64.hpp"

namespace itch::transport {

/// @brief Replays ITCH market data straight from a captured network trace.
///
/// Firms most often archive and share feeds as packet captures, so being able to
/// consume a `.pcap`/`.pcapng` file directly removes the need to pre-strip the
/// transport framing. The reader is implemented entirely in-house (no libpcap
/// dependency): it understands the classic pcap and the pcapng container formats,
/// walks the Ethernet / IPv4 / IPv6 / UDP layers of each captured frame, extracts
/// the UDP payload, and feeds it through an embedded `MoldUdp64Decoder`, which in
/// turn yields the decoded ITCH messages to the caller's callback.
///
/// @note Only offline capture files are supported. Live capture would require a
///       platform packet-capture library and is tracked as future work behind a
///       separate build option.
class PcapReader {
   public:
    /// @brief Constructs a reader that forwards each decoded ITCH message to
    ///        `callback`.
    explicit PcapReader(MessageCallback callback);

    /// @brief Decodes an in-memory capture buffer.
    ///
    /// @param capture The full contents of a `.pcap` or `.pcapng` file.
    /// @return `true` if the buffer was a recognized capture format, `false`
    ///         otherwise (in which case nothing was decoded).
    auto read(std::span<const std::byte> capture) -> bool;

    /// @brief Reads and decodes a capture file from disk.
    ///
    /// @return `true` on a recognized, readable file; `false` if the file cannot
    ///         be opened or is not a capture.
    auto read_file(const std::string& path) -> bool;

    /// @brief Restricts decoding to UDP datagrams sent to this destination port.
    ///        When unset, datagrams on any port are decoded.
    auto set_udp_port_filter(std::uint16_t port) -> void { m_port_filter = port; }

    /// @brief Clears any destination-port filter previously installed.
    auto clear_udp_port_filter() noexcept -> void { m_port_filter = std::nullopt; }

    /// @brief The embedded MoldUDP64 decoder (sequence tracking lives here).
    [[nodiscard]] auto mold_decoder() noexcept -> MoldUdp64Decoder& { return m_mold; }
    [[nodiscard]] auto mold_decoder() const noexcept -> const MoldUdp64Decoder& { return m_mold; }

    /// @brief Number of UDP datagrams extracted and handed to the MoldUDP64
    ///        decoder.
    [[nodiscard]] auto udp_datagrams() const noexcept -> std::uint64_t { return m_udp_datagrams; }

    /// @brief Total ITCH messages decoded across the whole capture.
    [[nodiscard]] auto messages_decoded() const noexcept -> std::uint64_t {
        return m_mold.messages_decoded();
    }

   private:
    // Container-format readers; each returns false if the buffer is not that
    // format or is structurally invalid.
    auto read_classic_pcap(std::span<const std::byte> capture, bool swapped) -> bool;
    auto read_pcapng(std::span<const std::byte> capture) -> bool;

    // Walks a single captured frame of the given link type and, if it carries a
    // matching UDP datagram, feeds the payload to the MoldUDP64 decoder.
    auto handle_frame(std::span<const std::byte> frame, std::uint32_t link_type) -> void;

    MoldUdp64Decoder             m_mold;
    std::optional<std::uint16_t> m_port_filter {};
    std::uint64_t                m_udp_datagrams {0};
};

}  // namespace itch::transport
