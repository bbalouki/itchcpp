#pragma once

#include <cstdint>
#include <limits>

#include "itch/price.hpp"

namespace itch::analytics {

/// @brief Running volume-weighted average price.
///
/// Accumulates the sum of price times shares and the total shares; the value is
/// their ratio. Use `reset` at the start of each interval for an interval VWAP.
class Vwap {
   public:
    /// @brief Adds an execution of `shares` at `price` to the accumulation.
    auto add(StandardPrice price, std::uint64_t shares) -> void {
        m_price_volume += price.to_double() * static_cast<double>(shares);
        m_volume += shares;
    }

    /// @brief The current VWAP, or NaN when no volume has been added.
    [[nodiscard]] auto value() const -> double {
        if (m_volume == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return m_price_volume / static_cast<double>(m_volume);
    }

    /// @brief The total shares accumulated so far.
    [[nodiscard]] auto volume() const noexcept -> std::uint64_t { return m_volume; }

    /// @brief Clears the accumulation (start a new interval).
    auto reset() noexcept -> void {
        m_price_volume = 0.0;
        m_volume       = 0;
    }

   private:
    double        m_price_volume {0.0};
    std::uint64_t m_volume {0};
};

/// @brief Running time-weighted average price.
///
/// Integrates the prevailing price over time: each sample contributes its price
/// weighted by the elapsed time since the previous sample. The value is the
/// time-weighted mean over the observed span.
class Twap {
   public:
    /// @brief Records that the price became `price` at `timestamp` (ns).
    auto add(StandardPrice price, std::uint64_t timestamp) -> void {
        if (m_has_sample && timestamp > m_last_timestamp) {
            const double elapsed = static_cast<double>(timestamp - m_last_timestamp);
            m_price_time += m_last_price * elapsed;
            m_total_time += elapsed;
        }
        m_last_price     = price.to_double();
        m_last_timestamp = timestamp;
        m_has_sample     = true;
    }

    /// @brief The current TWAP, or NaN when no span has elapsed.
    [[nodiscard]] auto value() const -> double {
        if (m_total_time <= 0.0) {
            return m_has_sample ? m_last_price : std::numeric_limits<double>::quiet_NaN();
        }
        return m_price_time / m_total_time;
    }

    /// @brief Clears the accumulation (start a new interval).
    auto reset() noexcept -> void {
        m_price_time     = 0.0;
        m_total_time     = 0.0;
        m_last_price     = 0.0;
        m_last_timestamp = 0;
        m_has_sample     = false;
    }

   private:
    double        m_price_time {0.0};
    double        m_total_time {0.0};
    double        m_last_price {0.0};
    std::uint64_t m_last_timestamp {0};
    bool          m_has_sample {false};
};

}  // namespace itch::analytics
