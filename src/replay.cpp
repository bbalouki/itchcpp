#include "itch/replay.hpp"

#include <chrono>
#include <thread>
#include <variant>

namespace itch {

namespace {

// Extracts the nanoseconds-past-midnight timestamp from any message.
auto timestamp_of(const Message& message) -> std::uint64_t {
    return std::visit([](const auto& concrete) { return concrete.timestamp; }, message);
}

}  // namespace

auto ReplayEngine::replay(std::span<const std::byte> data, const MessageCallback& callback) const
    -> std::uint64_t {
    using clock      = std::chrono::steady_clock;
    const bool paced = m_speed_multiplier > 0.0;

    std::uint64_t     replayed       = 0;
    bool              have_base      = false;
    std::uint64_t     base_timestamp = 0;
    clock::time_point base_wall {};

    Parser parser;
    parser.parse(data, [&](const Message& message) {
        if (paced) {
            const std::uint64_t feed_timestamp = timestamp_of(message);
            if (!have_base) {
                base_timestamp = feed_timestamp;
                base_wall      = clock::now();
                have_base      = true;
            } else if (feed_timestamp > base_timestamp) {
                // Scale the elapsed feed time by the speed multiplier and sleep
                // until that point relative to the first message's wall time.
                const double elapsed_ns =
                    static_cast<double>(feed_timestamp - base_timestamp) / m_speed_multiplier;
                const auto target =
                    base_wall + std::chrono::nanoseconds {static_cast<std::int64_t>(elapsed_ns)};
                std::this_thread::sleep_until(target);
            }
        }
        callback(message);
        ++replayed;
    });
    return replayed;
}

}  // namespace itch
