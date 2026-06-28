#pragma once

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
    RetransmitRequester()                                                  = default;
    RetransmitRequester(const RetransmitRequester&)                        = default;
    RetransmitRequester(RetransmitRequester&&) noexcept                    = default;
    auto operator=(const RetransmitRequester&) -> RetransmitRequester&     = default;
    auto operator=(RetransmitRequester&&) noexcept -> RetransmitRequester& = default;
    virtual ~RetransmitRequester()                                         = default;

    /// @brief Requests retransmission of `count` messages starting at sequence
    ///        `start_sequence` for the given session.
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
    auto set_gap_callback(GapCallback callback) -> void { m_gap_callback = std::move(callback); }

    /// @brief Installs a non-owning retransmission hook (nullptr clears it).
    auto set_retransmit_requester(RetransmitRequester* requester) noexcept -> void {
        m_requester = requester;
    }

    /// @brief The next sequence number expected for a session, if it has been seen.
    [[nodiscard]] auto expected_next(std::string_view session) const
        -> std::optional<std::uint64_t> {
        const auto iter = m_expected_by_session.find(std::string {session});
        if (iter == m_expected_by_session.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    /// @brief Total number of missing messages detected across all sessions.
    [[nodiscard]] auto gap_count() const noexcept -> std::uint64_t { return m_gap_count; }

    /// @brief Total number of distinct sequenced messages observed.
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
