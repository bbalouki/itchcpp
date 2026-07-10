#pragma once

/// @file
/// @brief OHLCV bar aggregation from the trade tape under configurable clocks.
///
/// `BarBuilder` consumes a stream of trades and emits completed bars whenever a
/// pluggable clock policy (time-, tick-, or volume-based) reports a new bucket,
/// letting time bars, tick bars, and volume bars share one implementation.
///
/// @author Bertin Balouki SIMYELI

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>

#include "itch/price.hpp"
#include "itch/tape.hpp"

namespace itch::analytics {

/// @brief One OHLCV bar aggregated from the trade tape.
struct Bar {
    std::uint64_t bucket {0};           ///< Clock bucket id this bar covers.
    std::uint64_t start_timestamp {0};  ///< Timestamp of the first trade in the bar.
    std::uint64_t end_timestamp {0};    ///< Timestamp of the last trade in the bar.
    StandardPrice open {};              ///< First trade price.
    StandardPrice high {};              ///< Highest trade price.
    StandardPrice low {};               ///< Lowest trade price.
    StandardPrice close {};             ///< Last trade price.
    std::uint64_t volume {0};           ///< Total shares traded in the bar.
    std::uint64_t trade_count {0};      ///< Number of trades in the bar.
};

/// @brief Callback invoked with each completed bar.
using BarCallback = std::function<void(const Bar&)>;

/// @brief A clock that buckets trades by wall-clock time (nanoseconds).
struct TimeClock {
    std::uint64_t      interval_ns {1};

    /// @brief Computes the bucket id for `trade` from its wall-clock timestamp.
    ///
    /// @param trade The trade whose timestamp determines the bucket.
    /// @return The bucket id, `trade.timestamp / interval_ns`.
    [[nodiscard]] auto bucket(const Trade& trade, std::uint64_t, std::uint64_t) const noexcept
        -> std::uint64_t {
        return trade.timestamp / interval_ns;
    }
};

/// @brief A clock that buckets a fixed number of trades into each bar.
struct TickClock {
    std::uint64_t      ticks_per_bar {1};

    /// @brief Computes the bucket id for the current trade from its tick position.
    ///
    /// @param tick_index 0-based index of the trade within the stream.
    /// @return The bucket id, `tick_index / ticks_per_bar`.
    [[nodiscard]] auto bucket(const Trade&, std::uint64_t, std::uint64_t tick_index) const noexcept
        -> std::uint64_t {
        return tick_index / ticks_per_bar;
    }
};

/// @brief A clock that buckets a fixed amount of traded volume into each bar.
struct VolumeClock {
    std::uint64_t      volume_per_bar {1};

    /// @brief Computes the bucket id for the current trade from cumulative volume.
    ///
    /// @param cumulative_volume Total shares traded so far, including this trade.
    /// @return The bucket id, `cumulative_volume / volume_per_bar`.
    [[nodiscard]] auto bucket(
        const Trade&, std::uint64_t cumulative_volume, std::uint64_t
    ) const noexcept -> std::uint64_t {
        return cumulative_volume / volume_per_bar;
    }
};

/// @brief Builds OHLCV bars from a trade stream under a configurable clock.
///
/// Feed trades in timestamp order with `add`; each time the clock's bucket id
/// changes, the completed bar is emitted to the callback. `flush` emits the final,
/// partial bar. The clock is a small policy (see `TimeClock`, `TickClock`,
/// `VolumeClock`) so time-, tick-, and volume-based aggregation share one builder.
template <typename Clock>
class BarBuilder {
   public:
    /// @brief Constructs a builder with the given clock policy and completion callback.
    ///
    /// @param clock Bucketing policy (e.g. `TimeClock`, `TickClock`, `VolumeClock`)
    ///              that decides when a bar completes.
    /// @param callback Function invoked with each completed `Bar`.
    BarBuilder(Clock clock, BarCallback callback)
        : m_clock {std::move(clock)}, m_callback {std::move(callback)} {}

    /// @brief Adds one trade, emitting the previous bar when the bucket changes.
    ///
    /// @param trade The next trade in timestamp order to fold into the current bar.
    auto add(const Trade& trade) -> void {
        const std::uint64_t bucket = m_clock.bucket(trade, m_cumulative_volume, m_tick_index);
        if (m_has_bar && bucket != m_bar.bucket) {
            emit();
        }
        if (!m_has_bar) {
            m_bar                 = Bar {};
            m_bar.bucket          = bucket;
            m_bar.start_timestamp = trade.timestamp;
            m_bar.open            = trade.price;
            m_bar.high            = trade.price;
            m_bar.low             = trade.price;
            m_has_bar             = true;
        }
        m_bar.high          = std::max(m_bar.high, trade.price);
        m_bar.low           = std::min(m_bar.low, trade.price);
        m_bar.close         = trade.price;
        m_bar.end_timestamp = trade.timestamp;
        m_bar.volume += trade.shares;
        ++m_bar.trade_count;

        m_cumulative_volume += trade.shares;
        ++m_tick_index;
    }

    /// @brief Emits the current partial bar, if any, and clears it.
    auto flush() -> void {
        if (m_has_bar) {
            emit();
        }
    }

   private:
    /// @brief Delivers the current bar to the callback and marks it inactive.
    auto emit() -> void {
        if (m_callback) {
            m_callback(m_bar);
        }
        m_has_bar = false;
    }

    Clock         m_clock;
    BarCallback   m_callback;
    Bar           m_bar {};
    bool          m_has_bar {false};
    std::uint64_t m_cumulative_volume {0};
    std::uint64_t m_tick_index {0};
};

}  // namespace itch::analytics
