#include <cstdint>
#include <format>
#include <iostream>
#include <span>
#include <vector>

#include "itch/analytics/microstructure.hpp"
#include "itch/book/book_manager.hpp"
#include "itch/encoder.hpp"
#include "itch/parser.hpp"

// Computes spread, mid price, queue imbalance, depth, and order-flow
// imbalance from a live `L3Book`, built from a small in-code sequence of
// resting orders so the example runs standalone without an external ITCH file.
auto main() -> int {
    constexpr std::uint64_t base_timestamp {34'200'000'000'000ULL};  // 09:30:00.000

    const std::vector<itch::Message> orders {
        itch::AddOrderMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp,
            .order_reference_number = 1,
            .buy_sell_indicator     = 'B',
            .shares                 = 100,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'499'000
        },
        itch::AddOrderMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + 100,
            .order_reference_number = 2,
            .buy_sell_indicator     = 'B',
            .shares                 = 200,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'498'000
        },
        itch::AddOrderMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + 200,
            .order_reference_number = 3,
            .buy_sell_indicator     = 'S',
            .shares                 = 150,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'501'000
        },
        itch::AddOrderMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + 300,
            .order_reference_number = 4,
            .buy_sell_indicator     = 'S',
            .shares                 = 250,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'502'000
        },
        itch::AddOrderMessage {
            .stock_locate           = 1,
            .tracking_number        = 0,
            .timestamp              = base_timestamp + 400,
            .order_reference_number = 5,
            .buy_sell_indicator     = 'B',
            .shares                 = 300,
            .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .price                  = 1'500'000  // Improves the best bid.
        },
    };

    std::vector<std::byte> buffer;
    for (const auto& order : orders) {
        const auto frame = itch::encode_frame(order);
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }

    itch::book::BookManager manager;
    manager.track_symbol("AAPL");

    std::vector<itch::book::Bbo> snapshots;
    manager.set_bbo_callback([&](const itch::book::L3Book&, const itch::book::Bbo& bbo) {
        snapshots.push_back(bbo);
    });

    itch::Parser parser;
    parser.parse(std::span<const std::byte> {buffer}, [&](const itch::Message& msg) {
        manager.process(msg);
    });

    const itch::book::L3Book* book = manager.book_for_symbol("AAPL");
    if (book == nullptr) {
        std::cerr << "Error: book for AAPL was not created.\n";
        return 1;
    }

    const itch::book::Bbo current = book->bbo();
    std::cout << std::format(
        "Spread: {:.4f}  Mid: {:.4f}  Queue imbalance: {:.4f}\n",
        itch::analytics::spread(current),
        itch::analytics::mid_price(current),
        itch::analytics::queue_imbalance(current)
    );

    const auto bid_depth = itch::analytics::depth_at_level(*book, itch::book::Side::buy, 2);
    const auto ask_depth = itch::analytics::depth_at_level(*book, itch::book::Side::sell, 2);
    std::cout << std::format("Depth (2 levels): bid={} ask={}\n", bid_depth, ask_depth);

    if (snapshots.size() >= 2) {
        const double flow_imbalance = itch::analytics::order_flow_imbalance(
            snapshots[snapshots.size() - 2], snapshots.back()
        );
        std::cout << std::format(
            "Order-flow imbalance (last BBO change): {:.2f}\n", flow_imbalance
        );
    }

    return 0;
}
