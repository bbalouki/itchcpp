#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>

#include "itch/indicators.hpp"
#include "itch/price.hpp"
#include "itch/time.hpp"

// The smallest, most beginner-friendly example in this directory: shows the
// three tiny utility types that keep raw ITCH wire values (integer prices,
// nanosecond timestamps, single-letter codes) from turning into silent
// off-by-a-decimal or look-the-code-up-yourself bugs in application code.
auto main() -> int {
    // A raw ITCH price is just an integer with an implied decimal point: the
    // same integer means different things depending on the field's scale, so
    // dividing by 10000 (or 100000000) by hand everywhere risks picking the
    // wrong divisor. StandardPrice/MwcbPrice bake the scale into the type.
    const itch::StandardPrice trade_price = itch::make_price(1500000);  // 4 implied decimals
    const itch::MwcbPrice     level1_price =
        itch::make_mwcb_price(410250000000ULL);  // 8 implied decimals
    std::cout << std::format(
        "Trade price raw {} -> {}\n", trade_price.raw(), trade_price.to_string()
    );
    std::cout << std::format(
        "MWCB level 1 raw {} -> {}\n", level1_price.raw(), level1_price.to_string()
    );

    // ITCH timestamps are raw nanoseconds since midnight with no date attached.
    // These helpers turn that offset into readable clock time, or a full
    // calendar time point given a session date, without hand-rolled div/mod math.
    constexpr std::uint64_t market_open_ns = 34'200'000'000'000ULL;  // 09:30:00.000000000
    std::cout << std::format(
        "Market open time of day: {}\n", itch::format_timestamp(market_open_ns)
    );
    const std::chrono::year_month_day session_date {
        std::chrono::year {2026}, std::chrono::month {7}, std::chrono::day {10}
    };
    std::cout << std::format(
        "Market open full timestamp: {}\n", itch::format_time_point(session_date, market_open_ns)
    );

    // Single-letter wire codes (event codes, trading states, ...) are meaningless
    // without a lookup table; indicators.hpp supplies compile-time tables instead
    // of scattering string literals through every call site that needs one.
    std::cout << std::format(
        "Event code 'O' means: {}\n", itch::indicators::SYSTEM_EVENT_CODES.at_or('O', "Unknown")
    );
    std::cout << std::format(
        "Trading state 'T' means: {}\n", itch::indicators::TRADING_STATES.at_or('T', "Unknown")
    );
    return 0;
}
