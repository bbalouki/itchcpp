#include <fstream>
#include <print>
#include <vector>

#include "itch/analytics/vwap.hpp"
#include "itch/book/book_manager.hpp"
#include "itch/parser.hpp"

// Computes the session VWAP for a chosen symbol by driving the BookManager's
// trade tape through a running Vwap accumulator.
auto main(int argc, char* argv[]) -> int {
    if (argc < 3) {
        std::println(stderr, "Usage: {} <itch_file> <symbol>", argv[0]);
        return 1;
    }

    std::ifstream file {argv[1], std::ios::binary};
    if (!file) {
        std::println(stderr, "Error: cannot open '{}'.", argv[1]);
        return 1;
    }
    std::vector<char> buffer {std::istreambuf_iterator<char> {file}, {}};

    itch::book::BookManager manager;
    manager.track_symbol(argv[2]);

    itch::analytics::Vwap vwap;
    manager.set_trade_callback([&](const itch::Trade& trade) {
        if (trade.symbol == argv[2]) {
            vwap.add(trade.price, trade.shares);
        }
    });

    itch::Parser parser;
    parser.parse(buffer.data(), buffer.size(), [&](const itch::Message& msg) {
        manager.process(msg);
    });

    std::println("VWAP for {}: {:.4f} over {} shares", argv[2], vwap.value(), vwap.volume());
    return 0;
}
