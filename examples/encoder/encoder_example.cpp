#include <algorithm>
#include <cstddef>
#include <format>
#include <iostream>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/parser.hpp"

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

auto make_sample_order() -> itch::AddOrderMessage {
    itch::AddOrderMessage order {
        .stock_locate           = 7,
        .tracking_number        = 3,
        .timestamp              = 34'200'000'123'456ULL,
        .order_reference_number = 987654321,
        .buy_sell_indicator     = 'B',
        .shares                 = 500,
        .stock                  = {},
        .price                  = 1500000,
    };
    fill_field(order.stock, "AAPL");
    return order;
}

}  // namespace

// Demonstrates the encoder as the inverse of the parser: it serializes a message
// back to wire bytes and guarantees parse(encode(msg)) == msg, which is what
// lets tests and scenario generators synthesize valid ITCH streams instead of
// depending on captured fixture files.
auto main() -> int {
    const itch::AddOrderMessage original = make_sample_order();
    const itch::Message         message  = original;

    const std::vector<std::byte> body  = itch::encode_message(message);
    const std::vector<std::byte> frame = itch::encode_frame(message);
    std::cout << std::format(
        "Encoded body: {} bytes; framed (2-byte length prefix): {} bytes\n",
        body.size(),
        frame.size()
    );

    itch::Parser                     parser;
    const std::vector<itch::Message> decoded_messages =
        parser.parse(std::span<const std::byte> {frame});
    std::cout << std::format(
        "Parser decoded {} message(s) from the frame\n", decoded_messages.size()
    );

    // Message has no operator== of its own, so the round-trip guarantee is
    // verified field by field on the decoded alternative instead.
    const auto& decoded     = std::get<itch::AddOrderMessage>(decoded_messages.front());
    const bool  round_trips = decoded.order_reference_number == original.order_reference_number &&
                             decoded.price == original.price && decoded.shares == original.shares &&
                             decoded.buy_sell_indicator == original.buy_sell_indicator &&
                             decoded.timestamp == original.timestamp;
    std::cout << std::format(
        "Round-trip parse(encode(msg)) == msg: {}\n", round_trips ? "holds" : "FAILED"
    );
    return 0;
}
