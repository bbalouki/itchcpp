#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/replay.hpp"

namespace {

// ITCH text fields are fixed-width and space-padded on the wire, not
// null-terminated C strings, so a plain string-literal designated initializer
// cannot fill them (there is no room for the implicit '\0'). This copies
// `value` in and pads the remainder with spaces instead.
template <std::size_t Size>
auto fill_field(char (&dest)[Size], std::string_view value) -> void {
    std::ranges::fill(dest, ' ');
    std::ranges::copy(value, dest);
}

auto make_order(std::uint32_t index, std::uint64_t session_start_ns, std::uint64_t message_gap_ns)
    -> itch::AddOrderMessage {
    itch::AddOrderMessage order {
        .stock_locate    = 1,
        .tracking_number = 1,
        .timestamp       = session_start_ns + static_cast<std::uint64_t>(index) * message_gap_ns,
        .order_reference_number = 1000ULL + static_cast<std::uint64_t>(index),
        .buy_sell_indicator     = 'B',
        .shares                 = 100,
        .stock                  = {},
        .price                  = 1'500'000U + (index * 100U),
    };
    fill_field(order.stock, "AAPL");
    return order;
}

}  // namespace

// Demonstrates ReplayEngine pacing a synthesized feed by its own timestamps
// rather than delivering it as fast as the CPU can parse, which is what makes
// it useful for realistic backtesting and system simulation instead of a plain
// Parser::parse loop.
auto main() -> int {
    constexpr std::uint64_t session_start_ns = 34'200'000'000'000ULL;  // 09:30:00.000000000
    constexpr std::uint64_t message_gap_ns   = 100'000'000ULL;         // 100 ms apart in feed time
    constexpr std::uint32_t order_count      = 4;

    std::vector<std::byte> stream;
    for (std::uint32_t index = 0; index < order_count; ++index) {
        const itch::AddOrderMessage  order = make_order(index, session_start_ns, message_gap_ns);
        const std::vector<std::byte> frame = itch::encode_frame(itch::Message {order});
        stream.insert(stream.end(), frame.begin(), frame.end());
    }

    // At 200x speed, the 100 ms feed-time gaps between messages become ~0.5 ms
    // wall-clock sleeps, so the example finishes almost instantly while still
    // exercising real pacing rather than a no-op.
    itch::ReplayEngine  engine {200.0};
    std::uint64_t       messages_seen = 0;
    const auto          start         = std::chrono::steady_clock::now();
    const std::uint64_t replayed =
        engine.replay(std::span<const std::byte> {stream}, [&](const itch::Message& message) {
            ++messages_seen;
            const auto& order = std::get<itch::AddOrderMessage>(message);
            std::cout << std::format(
                "Replayed order #{} at {} ns past midnight\n", messages_seen, order.timestamp
            );
        });
    const auto elapsed = std::chrono::steady_clock::now() - start;

    std::cout << std::format(
        "Replayed {} messages in {} ms wall-clock at {}x speed\n",
        replayed,
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
        engine.speed()
    );
    return 0;
}
