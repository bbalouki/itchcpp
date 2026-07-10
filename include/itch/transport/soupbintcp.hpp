#pragma once

/// @file
/// @brief Stateful decoder for the SoupBinTCP session-layer protocol.
///
/// SoupBinTCP is NASDAQ's reliable, ordered TCP framing used for the Glimpse
/// snapshot service and for sequenced/unsequenced ITCH message delivery and
/// replay. This header declares the packet type enumeration and the
/// `SoupBinDecoder` that reassembles a raw byte stream into discrete packets
/// and decoded ITCH messages.
///
/// @author Bertin Balouki SIMYELI

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "itch/parser.hpp"
#include "itch/transport/sequencing.hpp"

namespace itch::transport {

/// @brief The SoupBinTCP packet types (the one-byte type that follows the
///        2-byte length prefix).
///
/// SoupBinTCP is the reliable, ordered TCP framing NASDAQ uses for the Glimpse
/// snapshot service and for recovery/replay. Only the server-to-client subset is
/// needed to consume a captured or replayed stream, but every defined type is
/// listed for completeness.
enum class SoupBinPacketType : char {
    debug            = '+',  ///< Free-form debug text (either direction).
    login_accepted   = 'A',  ///< Session id (10) + starting sequence number (20, ASCII).
    login_rejected   = 'J',  ///< Reject reason code (1).
    sequenced_data   = 'S',  ///< One sequenced application (ITCH) message.
    server_heartbeat = 'H',  ///< Keep-alive from the server.
    end_of_session   = 'Z',  ///< The server has finished the session.
    login_request    = 'L',  ///< Client login request.
    unsequenced_data = 'U',  ///< One unsequenced application message.
    client_heartbeat = 'R',  ///< Keep-alive from the client.
    logout_request   = 'O',  ///< Client logout request.
};

/// @brief A stateful decoder for a SoupBinTCP byte stream.
///
/// Because TCP delivers a byte stream rather than discrete packets, the decoder
/// accumulates input across `feed` calls and emits messages only for complete
/// packets. Sequenced (and, optionally, unsequenced) data packets each carry a
/// single ITCH message, which is decoded through an internal `Parser` and handed
/// to the `MessageCallback`. Control packets (login, heartbeat, logout, end of
/// session) are surfaced through the optional event callback. The starting
/// sequence number is learned from the Login Accepted packet and advanced per
/// sequenced message, with gaps reported through the embedded `SequenceTracker`.
class SoupBinDecoder {
   public:
    /// @brief Invoked for each non-data control packet, with its raw payload.
    using EventCallback =
        std::function<void(SoupBinPacketType type, std::span<const std::byte> payload)>;

    /// @brief Constructs a decoder that calls `callback` for each ITCH message.
    ///
    /// @param callback Invoked with each ITCH message decoded from a
    ///        sequenced or unsequenced data packet.
    explicit SoupBinDecoder(MessageCallback callback);

    /// @brief Feeds a chunk of the TCP byte stream, processing any complete
    ///        packets it completes.
    ///
    /// @param bytes The next contiguous chunk of bytes read from the TCP
    ///        connection.
    auto feed(std::span<const std::byte> bytes) -> void;

    /// @brief Installs the control-packet event callback (empty clears it).
    ///
    /// @param callback Invoked for each non-data control packet decoded.
    auto set_event_callback(EventCallback callback) -> void;

    /// @brief The embedded sequence tracker (install gap callbacks here).
    /// @return Reference to the embedded `SequenceTracker`.
    [[nodiscard]] auto tracker() noexcept -> SequenceTracker& { return m_tracker; }
    /// @brief The embedded sequence tracker (install gap callbacks here).
    /// @return Const reference to the embedded `SequenceTracker`.
    [[nodiscard]] auto tracker() const noexcept -> const SequenceTracker& { return m_tracker; }

    /// @brief The session id learned from Login Accepted (empty until then).
    /// @return The current session id, or an empty view before login.
    [[nodiscard]] auto current_session() const -> std::string_view { return m_session; }

    /// @brief The next sequence number the decoder expects for a data packet.
    /// @return The next expected sequence number.
    [[nodiscard]] auto next_sequence() const noexcept -> std::uint64_t { return m_next_sequence; }

    /// @brief Total sequenced/unsequenced data messages decoded.
    /// @return The total count of decoded data messages.
    [[nodiscard]] auto messages_decoded() const noexcept -> std::uint64_t {
        return m_messages_decoded;
    }

   private:
    /// @brief Decodes the single packet (type byte plus payload) and
    ///        dispatches it to the event callback or the application decoder.
    ///
    /// @param type The one-byte SoupBinTCP packet type.
    /// @param payload The packet body following the type byte.
    auto process_packet(SoupBinPacketType type, std::span<const std::byte> payload) -> void;

    /// @brief Decodes one application message payload through the parser and
    ///        advances the expected sequence number.
    ///
    /// @param payload The raw ITCH message bytes carried by a sequenced or
    ///        unsequenced data packet.
    auto decode_application_message(std::span<const std::byte> payload) -> void;

    Parser                 m_parser {};
    MessageCallback        m_callback;
    EventCallback          m_event_callback {};
    SequenceTracker        m_tracker {};
    std::vector<std::byte> m_buffer {};
    std::string            m_session {};
    std::uint64_t          m_next_sequence {1};
    std::uint64_t          m_messages_decoded {0};
};

}  // namespace itch::transport
