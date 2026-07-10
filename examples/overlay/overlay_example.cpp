#include <cstddef>
#include <format>
#include <iostream>
#include <vector>

#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/overlay.hpp"

// Demonstrates itch::overlay::for_each_message: a zero-copy, lazy alternative
// to itch::Parser. The eager Parser decodes every field of every message up
// front; the overlay view only converts the specific fields the callback
// touches, which pays off when a caller only cares about a few fields.
auto main() -> int {
    const itch::Message first_order = itch::AddOrderMessage {
        .stock_locate           = 1,
        .tracking_number        = 0,
        .timestamp              = 34200000000000ULL,
        .order_reference_number = 3001,
        .buy_sell_indicator     = 'B',
        .shares                 = 100,
        .stock                  = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .price                  = 1500000,
    };
    const itch::Message second_order = itch::AddOrderMessage {
        .stock_locate           = 2,
        .tracking_number        = 0,
        .timestamp              = 34200100000000ULL,
        .order_reference_number = 3002,
        .buy_sell_indicator     = 'S',
        .shares                 = 200,
        .stock                  = {'M', 'S', 'F', 'T', ' ', ' ', ' ', ' '},
        .price                  = 3200000,
    };
    const itch::Message third_order = itch::AddOrderMessage {
        .stock_locate           = 3,
        .tracking_number        = 0,
        .timestamp              = 34200200000000ULL,
        .order_reference_number = 3003,
        .buy_sell_indicator     = 'B',
        .shares                 = 50,
        .stock                  = {'G', 'O', 'O', 'G', ' ', ' ', ' ', ' '},
        .price                  = 2750000,
    };

    std::vector<std::byte> buffer;
    for (const auto& message : {first_order, second_order, third_order}) {
        const auto frame = itch::encode_frame(message);
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }

    std::uint64_t viewed_count = 0;
    const auto    delivered =
        itch::overlay::for_each_message(buffer, [&](const itch::overlay::MessageView& view) {
            if (view.type() != 'A') {
                return;
            }
            ++viewed_count;
            // Only stock() and price() are decoded here; every other field of
            // this Add Order frame (shares, side, order reference number, ...)
            // is never converted from network byte order, unlike a Parser pass.
            const itch::overlay::AddOrderView order {view.data(), view.size()};
            std::cout << std::format(
                "order {}: {} @ {}\n", viewed_count, order.stock(), order.price()
            );
        });

    std::cout << std::format(
        "{} of {} frames viewed as Add Order messages\n", viewed_count, delivered
    );
    return 0;
}
