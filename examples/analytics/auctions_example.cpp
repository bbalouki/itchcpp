#include <cstdint>
#include <format>
#include <iostream>
#include <span>
#include <vector>

#include "itch/analytics/auctions.hpp"
#include "itch/analytics/imbalance.hpp"
#include "itch/encoder.hpp"
#include "itch/parser.hpp"

// Reconstructs an opening auction from the NOII imbalance messages leading up
// to a cross with `AuctionTracker`, alongside decoding a standalone NOII
// message directly with `make_imbalance_info`.
auto main() -> int {
    constexpr std::uint64_t base_timestamp {34'200'000'000'000ULL};  // 09:30:00.000

    const itch::NOIIMessage first_noii {
        .stock_locate              = 1,
        .tracking_number           = 0,
        .timestamp                 = base_timestamp,
        .paired_shares             = 50'000,
        .imbalance_shares          = 5'000,
        .imbalance_direction       = 'B',
        .stock                     = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .far_price                 = 1'499'500,
        .near_price                = 1'500'000,
        .current_reference_price   = 1'500'000,
        .cross_type                = 'O',
        .price_variation_indicator = 'L'
    };

    // Demonstrate decoding a single NOII message directly.
    const itch::analytics::ImbalanceInfo decoded = itch::analytics::make_imbalance_info(first_noii);
    std::cout << std::format(
        "NOII for {}: {} shares imbalance ({}), reference price {:.4f}\n",
        decoded.stock,
        decoded.imbalance_shares,
        itch::analytics::imbalance_direction_name(decoded.imbalance_direction),
        decoded.current_reference_price
    );

    const std::vector<itch::Message> stream {
        itch::Message {first_noii},
        itch::Message {itch::NOIIMessage {
            .stock_locate              = 1,
            .tracking_number           = 0,
            .timestamp                 = base_timestamp + 1'000'000'000ULL,
            .paired_shares             = 60'000,
            .imbalance_shares          = 2'000,
            .imbalance_direction       = 'B',
            .stock                     = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .far_price                 = 1'500'000,
            .near_price                = 1'500'000,
            .current_reference_price   = 1'500'000,
            .cross_type                = 'O',
            .price_variation_indicator = 'L'
        }},
        itch::Message {itch::CrossTradeMessage {
            .stock_locate    = 1,
            .tracking_number = 0,
            .timestamp       = base_timestamp + 2'000'000'000ULL,
            .shares          = 62'000,
            .stock           = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
            .cross_price     = 1'500'000,
            .match_number    = 1,
            .cross_type      = 'O'
        }},
    };

    std::vector<std::byte> buffer;
    for (const auto& msg : stream) {
        const auto frame = itch::encode_frame(msg);
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }

    itch::analytics::AuctionTracker tracker;
    tracker.set_auction_callback([](const itch::analytics::Auction& auction) {
        std::cout << std::format(
            "{} for {}: {} shares @ {:.4f} (paired {}, imbalance {})\n",
            itch::analytics::cross_type_name(auction.cross_type),
            auction.stock,
            auction.cross_shares,
            auction.cross_price,
            auction.paired_shares,
            auction.imbalance_shares
        );
    });

    itch::Parser parser;
    parser.parse(std::span<const std::byte> {buffer}, [&](const itch::Message& msg) {
        tracker.process(msg);
    });

    return 0;
}
