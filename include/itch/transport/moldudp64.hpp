#pragma once

/// @file
/// @brief Decoder for NASDAQ's MoldUDP64 UDP multicast market-data framing.
///
/// This header declares the `MoldUdp64Header` wire structure and the
/// `MoldUdp64Decoder` that strips it from each datagram and feeds the
/// contained, length-prefixed ITCH message blocks through the shared
/// `Parser`.
///
/// @author Bertin Balouki SIMYELI

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "itch/parser.hpp"
#include "itch/transport/sequencing.hpp"

namespace itch::transport {

/// @brief The fixed 20-byte header that prefixes every MoldUDP64 downstream
///        packet.
///
/// MoldUDP64 is the lightweight UDP multicast framing NASDAQ uses to disseminate
/// live market data. Each datagram carries a session identifier, the sequence
/// number of the first message it contains, and a count of the message blocks
/// that follow. The message blocks themselves are length-prefixed exactly like a
/// raw ITCH stream, so once the header is stripped the remainder feeds straight
/// into the ordinary `Parser` framing loop.
struct MoldUdp64Header {
    std::array<char, 10> session {};           ///< Session id, space padded.
    std::uint64_t        sequence_number {0};  ///< Sequence of the first message block.
    std::uint16_t        message_count {0};    ///< Number of message blocks in the packet.

    /// @brief Sentinel `message_count` indicating an end-of-session packet.
    static constexpr std::uint16_t END_OF_SESSION = 0xFFFF;

    /// @brief The session id as a view with trailing padding removed.
    /// @return A view of the session id with trailing spaces/NULs stripped.
    [[nodiscard]] auto session_view() const -> std::string_view {
        std::size_t length = session.size();
        while (length > 0 && (session[length - 1] == ' ' || session[length - 1] == '\0')) {
            --length;
        }
        return std::string_view {session.data(), length};
    }

    /// @brief A heartbeat carries no message blocks (`message_count == 0`).
    /// @return True if the packet is a heartbeat, false otherwise.
    [[nodiscard]] auto is_heartbeat() const noexcept -> bool { return message_count == 0; }

    /// @brief End-of-session is signalled by the sentinel `message_count`.
    /// @return True if the packet signals end of session, false otherwise.
    [[nodiscard]] auto is_end_of_session() const noexcept -> bool {
        return message_count == END_OF_SESSION;
    }
};

/// @brief Decodes MoldUDP64 datagrams and forwards the contained ITCH messages.
///
/// One instance decodes a stream of datagrams from one or more sessions: call
/// `decode_packet` once per UDP payload. Each well-formed data packet's message
/// blocks are handed to an internal `Parser`, which invokes the supplied
/// `MessageCallback` for every decoded ITCH message. Per-packet sequence numbers
/// are fed to an embedded `SequenceTracker` so gaps in the multicast stream are
/// surfaced to the caller.
class MoldUdp64Decoder {
   public:
    /// @brief The on-wire size of the MoldUDP64 packet header, in bytes.
    static constexpr std::size_t HEADER_SIZE = 20;

    /// @brief Constructs a decoder that calls `callback` for each ITCH message.
    /// @param callback Invoked with each ITCH message decoded from a data
    ///        packet's message blocks.
    explicit MoldUdp64Decoder(MessageCallback callback);

    /// @brief Decodes a single MoldUDP64 datagram.
    ///
    /// @param packet The full UDP payload (header plus message blocks).
    /// @return The parsed header, or `std::nullopt` if the datagram is too short
    ///         to contain a valid header.
    auto decode_packet(std::span<const std::byte> packet) -> std::optional<MoldUdp64Header>;

    /// @brief The embedded sequence tracker (install gap callbacks here).
    /// @return Reference to the embedded `SequenceTracker`.
    [[nodiscard]] auto tracker() noexcept -> SequenceTracker& { return m_tracker; }
    /// @brief The embedded sequence tracker (install gap callbacks here).
    /// @return Const reference to the embedded `SequenceTracker`.
    [[nodiscard]] auto tracker() const noexcept -> const SequenceTracker& { return m_tracker; }

    /// @brief Total number of datagrams passed to `decode_packet`.
    /// @return The count of datagrams decoded so far.
    [[nodiscard]] auto packets_decoded() const noexcept -> std::uint64_t {
        return m_packets_decoded;
    }

    /// @brief Total number of ITCH messages decoded from all datagrams.
    /// @return The total count of decoded ITCH messages.
    [[nodiscard]] auto messages_decoded() const noexcept -> std::uint64_t {
        return m_messages_decoded;
    }

   private:
    Parser          m_parser {};
    MessageCallback m_callback;
    SequenceTracker m_tracker {};
    std::uint64_t   m_packets_decoded {0};
    std::uint64_t   m_messages_decoded {0};
};

}  // namespace itch::transport
