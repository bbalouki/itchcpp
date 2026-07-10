#pragma once

/// @file
/// @brief Per-session sequence-gap detection shared by the MoldUDP64 and
///        SoupBinTCP transport decoders.
///
/// This header declares the `RetransmitRequester` hook a caller implements to
/// drive gap recovery, and the `SequenceTracker` that watches per-packet
/// sequence numbers and reports gaps through a callback and/or the requester.
///
/// @author Bertin Balouki SIMYELI

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace itch::transport {

/// @brief Abstract hook a caller implements to drive recovery against a replay
///        or retransmission service when a sequence gap is detected.
///
/// The library never performs network I/O itself: when `SequenceTracker` sees a
/// gap it calls `request_retransmit`, and the caller is responsible for issuing
/// the actual re-request (for example a SoupBinTCP/MoldUDP64 rewind request) and
/// feeding the recovered messages back through the decoder.
class RetransmitRequester {
   public:
    /// @brief Constructs a requester with no state.
    RetransmitRequester() = default;
    /// @brief Copy-constructs a requester.
    /// @param other The requester to copy.
    RetransmitRequester(const RetransmitRequester&) = default;
    /// @brief Move-constructs a requester.
    /// @param other The requester to move from.
    RetransmitRequester(RetransmitRequester&&) noexcept = default;
    /// @brief Copy-assigns a requester.
    /// @param other The requester to copy.
    /// @return Reference to this requester.
    auto operator=(const RetransmitRequester&) -> RetransmitRequester& = default;
    /// @brief Move-assigns a requester.
    /// @param other The requester to move from.
    /// @return Reference to this requester.
    auto operator=(RetransmitRequester&&) noexcept -> RetransmitRequester& = default;
    /// @brief Destroys the requester.
    virtual ~RetransmitRequester() = default;

    /// @brief Requests retransmission of `count` messages starting at sequence
    ///        `start_sequence` for the given session.
    ///
    /// @param session The session identifier the gap was observed on.
    /// @param start_sequence The first missing sequence number to retransmit.
    /// @param count The number of missing messages to retransmit.
    virtual auto request_retransmit(
        std::string_view session, std::uint64_t start_sequence, std::uint64_t count
    ) -> void = 0;
};

/// @brief Tracks per-session sequence numbers across a transport layer and
///        surfaces gaps.
///
/// Both the MoldUDP64 and SoupBinTCP decoders feed their per-packet sequence
/// information here. The tracker maintains, per session, the next sequence number
/// it expects to see. When an observed packet starts ahead of that expectation it
/// reports a gap (through the gap callback and the optional `RetransmitRequester`)
/// and resynchronizes. Duplicate or already-seen sequences are ignored so a
/// replayed recovery stream does not double-count.
class SequenceTracker {
   public:
    /// @brief Invoked once per detected gap with the session, the sequence the
    ///        tracker expected, and the (higher) sequence actually received.
    using GapCallback = std::function<
        void(std::string_view session, std::uint64_t expected, std::uint64_t received)>;

    /// @brief Records that a packet beginning at `first_sequence` carried `count`
    ///        sequenced messages for `session`.
    ///
    /// @param session The session identifier the packet belongs to.
    /// @param first_sequence The sequence number of the first message in the
    ///        packet.
    /// @param count The number of sequenced messages carried by the packet.
    /// @return The number of missing messages detected (0 when in order).
    auto observe(std::string_view session, std::uint64_t first_sequence, std::uint64_t count)
        -> std::uint64_t {
        const std::string key {session};
        auto              iter = m_expected_by_session.find(key);
        if (iter == m_expected_by_session.end()) {
            // First time we see this session: anchor on its first sequence.
            m_expected_by_session.emplace(key, first_sequence + count);
            m_messages_seen += count;
            return 0;
        }

        std::uint64_t& expected = iter->second;
        std::uint64_t  gap      = 0;
        if (first_sequence > expected) {
            gap = first_sequence - expected;
            m_gap_count += gap;
            if (m_gap_callback) {
                m_gap_callback(session, expected, first_sequence);
            }
            if (m_requester != nullptr) {
                m_requester->request_retransmit(session, expected, gap);
            }
        }

        const std::uint64_t end_sequence = first_sequence + count;
        if (end_sequence > expected) {
            // Count only the genuinely new messages, not replayed duplicates.
            const std::uint64_t already_seen =
                expected > first_sequence ? expected - first_sequence : 0;
            m_messages_seen += count - std::min(count, already_seen);
            expected = end_sequence;
        }
        return gap;
    }

    /// @brief Installs the gap-notification callback (empty clears it).
    /// @param callback Invoked once per detected gap.
    auto set_gap_callback(GapCallback callback) -> void { m_gap_callback = std::move(callback); }

    /// @brief Installs a non-owning retransmission hook (nullptr clears it).
    /// @param requester Non-owning pointer to the requester to notify on
    ///        gaps, or nullptr to clear it.
    auto set_retransmit_requester(RetransmitRequester* requester) noexcept -> void {
        m_requester = requester;
    }

    /// @brief The next sequence number expected for a session, if it has been seen.
    /// @param session The session identifier to look up.
    /// @return The next expected sequence number, or `std::nullopt` if the
    ///         session has not been observed.
    [[nodiscard]] auto expected_next(std::string_view session
    ) const -> std::optional<std::uint64_t> {
        const auto iter = m_expected_by_session.find(std::string {session});
        if (iter == m_expected_by_session.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    /// @brief Total number of missing messages detected across all sessions.
    /// @return The total gap count.
    [[nodiscard]] auto gap_count() const noexcept -> std::uint64_t { return m_gap_count; }

    /// @brief Total number of distinct sequenced messages observed.
    /// @return The total count of distinct messages observed.
    [[nodiscard]] auto messages_seen() const noexcept -> std::uint64_t { return m_messages_seen; }

    /// @brief Forgets all per-session state and resets the counters.
    auto reset() -> void {
        m_expected_by_session.clear();
        m_gap_count     = 0;
        m_messages_seen = 0;
    }

   private:
    std::unordered_map<std::string, std::uint64_t> m_expected_by_session {};
    GapCallback                                    m_gap_callback {};
    RetransmitRequester*                           m_requester {nullptr};
    std::uint64_t                                  m_gap_count {0};
    std::uint64_t                                  m_messages_seen {0};
};

}  // namespace itch::transport
