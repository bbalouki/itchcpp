#pragma once

/// @file
/// @brief A timestamp-paced replay engine for previously captured ITCH streams.
///
/// Wraps the `Parser` so a consumer can play back a buffer at (a multiple of)
/// its original wall-clock cadence rather than as fast as the CPU can parse
/// it, which is useful for realistic backtesting and system simulation.
///
/// @author Bertin Balouki SIMYELI

#include <cstdint>
#include <span>

#include "itch/parser.hpp"

namespace itch {

/// @brief Replays a parsed ITCH stream paced by message timestamps.
///
/// For realistic backtesting and system simulation, a consumer often needs the
/// feed delivered at its original wall-clock cadence rather than as fast as the
/// CPU can parse it. The replay engine parses a buffer and invokes the callback
/// for each message, sleeping between messages so the inter-message gaps match the
/// differences in their nanoseconds-past-midnight timestamps, optionally scaled by
/// a speed factor.
class ReplayEngine {
   public:
    /// @brief Constructs an engine with a speed multiplier.
    ///
    /// @param speed_multiplier Wall-clock speed relative to the original feed: 1.0
    ///        replays in real time, 2.0 twice as fast, 0.5 half speed. A value
    ///        less than or equal to 0 replays with no pacing (as fast as possible).
    explicit ReplayEngine(double speed_multiplier = 1.0) noexcept
        : m_speed_multiplier {speed_multiplier} {}

    /// @brief Sets the speed multiplier (see the constructor).
    ///
    /// @param speed_multiplier Wall-clock speed relative to the original feed
    ///        (see the constructor for the exact semantics).
    auto set_speed(double speed_multiplier) noexcept -> void {
        m_speed_multiplier = speed_multiplier;
    }

    /// @brief The current speed multiplier.
    ///
    /// @return The currently configured speed multiplier.
    [[nodiscard]] auto speed() const noexcept -> double { return m_speed_multiplier; }

    /// @brief Parses `data` and invokes `callback` for each message, paced by the
    ///        message timestamps.
    ///
    /// @param data A view over the contiguous buffer containing ITCH data.
    /// @param callback A function to be called for each successfully parsed message.
    /// @return The number of messages replayed.
    auto replay(std::span<const std::byte> data, const MessageCallback& callback) const
        -> std::uint64_t;

   private:
    double m_speed_multiplier {1.0};
};

}  // namespace itch
