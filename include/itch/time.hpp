#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <string>

namespace itch {

/// @brief A system-clock time point at nanosecond resolution.
using ItchTimePoint = std::chrono::sys_time<std::chrono::nanoseconds>;

/// @brief Combines a session date with an ITCH timestamp into a time point.
///
/// ITCH timestamps are raw nanoseconds elapsed since midnight, with no date or
/// time-zone attached. This helper anchors that offset to the midnight of a
/// caller-supplied session date. No time-zone conversion is performed: the
/// result is expressed in the same frame as the date you pass in (pass a UTC
/// date for a UTC result, an Eastern-time date for an Eastern result).
///
/// @param session_date The calendar date whose midnight anchors the timestamp.
/// @param nanos_past_midnight The raw ITCH timestamp (nanoseconds past midnight).
/// @return The absolute time point for the event.
[[nodiscard]] constexpr auto to_time_point(
    std::chrono::year_month_day session_date, std::uint64_t nanos_past_midnight
) noexcept -> ItchTimePoint {
    return std::chrono::sys_days {session_date} + std::chrono::nanoseconds {nanos_past_midnight};
}

/// @brief Formats a raw ITCH timestamp as a time-of-day string.
///
/// @param nanos_past_midnight The raw ITCH timestamp (nanoseconds past midnight).
/// @return A string of the form "HH:MM:SS.nnnnnnnnn".
[[nodiscard]] inline auto format_timestamp(std::uint64_t nanos_past_midnight) -> std::string {
    const std::chrono::hh_mm_ss time_of_day {std::chrono::nanoseconds {nanos_past_midnight}};
    return std::format("{:%T}", time_of_day);
}

/// @brief Formats a session date plus an ITCH timestamp as a full date-time.
///
/// @param session_date The calendar date whose midnight anchors the timestamp.
/// @param nanos_past_midnight The raw ITCH timestamp (nanoseconds past midnight).
/// @return A string of the form "YYYY-MM-DD HH:MM:SS.nnnnnnnnn".
[[nodiscard]] inline auto format_time_point(
    std::chrono::year_month_day session_date, std::uint64_t nanos_past_midnight
) -> std::string {
    return std::format("{:%F %T}", to_time_point(session_date, nanos_past_midnight));
}

}  // namespace itch
