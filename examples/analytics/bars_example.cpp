#include <cstdint>
#include <format>
#include <iostream>
#include <span>
#include <vector>

#include "itch/analytics/bars.hpp"
#include "itch/book/book_manager.hpp"
#include "itch/encoder.hpp"
#include "itch/parser.hpp"

// Aggregates a synthetic trade tape into one-minute OHLCV bars with
// `BarBuilder` under a `TimeClock` policy, showing how the builder emits a
// completed bar each time the clock's bucket id advances.
auto main() -> int {
    constexpr std::uint64_t base_timestamp {34'200'000'000'000ULL};  // 09:30:00.000
    constexpr std::uint64_t one_minute_ns {60'000'000'000ULL};

    const std::vector<itch::Message> trades {
        itch::NonCrossTradeMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp,
            .order_reference_number = 1,
            .buy_sell_indicator     = 'B',
            .shares                 = 100,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'500'000,
            .match_number           = 1
        },
        itch::NonCrossTradeMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + 10'000'000'000ULL,
            .order_reference_number = 2,
            .buy_sell_indicator     = 'S',
            .shares                 = 200,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'501'000,
            .match_number           = 2
        },
        itch::NonCrossTradeMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + one_minute_ns + 10'000'000'000ULL,
            .order_reference_number = 3,
            .buy_sell_indicator     = 'B',
            .shares                 = 150,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'499'000,
            .match_number           = 3
        },
        itch::NonCrossTradeMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + one_minute_ns + 30'000'000'000ULL,
            .order_reference_number = 4,
            .buy_sell_indicator     = 'S',
            .shares                 = 300,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'502'000,
            .match_number           = 4
        },
        itch::NonCrossTradeMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + (2 * one_minute_ns) + 10'000'000'000ULL,
            .order_reference_number = 5,
            .buy_sell_indicator     = 'B',
            .shares                 = 100,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'503'000,
            .match_number           = 5
        },
    };

    std::vector<std::byte> buffer;
    for (const auto& trade : trades) {
        const auto frame = itch::encode_frame(trade);
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }

    itch::analytics::BarBuilder<itch::analytics::TimeClock> bars {
        itch::analytics::TimeClock {.interval_ns = one_minute_ns},
        [](const itch::analytics::Bar& bar) {
            std::cout << std::format(
                "Bar {}: open={:.4f} high={:.4f} low={:.4f} close={:.4f} volume={} trades={}\n",
                bar.bucket,
                bar.open,
                bar.high,
                bar.low,
                bar.close,
                bar.volume,
                bar.trade_count
            );
        }
    };

    itch::book::BookManager manager;
    manager.set_trade_callback([&](const itch::Trade& trade) { bars.add(trade); });

    itch::Parser parser;
    parser.parse(std::span<const std::byte> {buffer}, [&](const itch::Message& msg) {
        manager.process(msg);
    });
    bars.flush();  // Emit the final, partial bar.

    return 0;
}
