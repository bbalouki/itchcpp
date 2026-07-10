#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <span>
#include <vector>

#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/parser.hpp"
#include "itch/replay.hpp"
#include "itch/venue.hpp"

namespace {

auto fill_stock(char (&dest)[itch::STOCK_LEN], std::string_view symbol) -> void {
    std::memset(dest, ' ', itch::STOCK_LEN);
    std::memcpy(
        dest, symbol.data(), std::min(symbol.size(), static_cast<std::size_t>(itch::STOCK_LEN))
    );
}

// Round-trips a message through encode -> parse and returns the decoded variant.
auto round_trip(const itch::Message& message) -> itch::Message {
    const std::vector<std::byte> frame = itch::encode_frame(message);
    itch::Parser                 parser;
    std::vector<itch::Message>   out = parser.parse(std::span<const std::byte> {frame});
    EXPECT_EQ(out.size(), 1U);
    return out.front();
}

}  // namespace

TEST(Encoder, AddOrderRoundTrips) {
    itch::AddOrderMessage original {};
    original.stock_locate    = 7;
    original.tracking_number = 3;
    original.timestamp       = 0x0000123456789ABCULL & 0x0000FFFFFFFFFFFFULL;  // fits 48 bits
    original.order_reference_number = 987654321;
    original.buy_sell_indicator     = 'B';
    original.shares                 = 500;
    fill_stock(original.stock, "AAPL");
    original.price = 1500000;

    const auto decoded = std::get<itch::AddOrderMessage>(round_trip(itch::Message {original}));
    EXPECT_EQ(decoded.stock_locate, original.stock_locate);
    EXPECT_EQ(decoded.tracking_number, original.tracking_number);
    EXPECT_EQ(decoded.timestamp, original.timestamp);
    EXPECT_EQ(decoded.order_reference_number, original.order_reference_number);
    EXPECT_EQ(decoded.buy_sell_indicator, 'B');
    EXPECT_EQ(decoded.shares, 500U);
    EXPECT_EQ(itch::to_string(decoded.stock, itch::STOCK_LEN), "AAPL");
    EXPECT_EQ(decoded.price, 1500000U);
}

TEST(Encoder, NoiiRoundTripsAllFields) {
    itch::NOIIMessage original {};
    original.stock_locate        = 11;
    original.timestamp           = 42;
    original.paired_shares       = 100000;
    original.imbalance_shares    = 2500;
    original.imbalance_direction = 'B';
    fill_stock(original.stock, "MSFT");
    original.far_price                 = 3000000;
    original.near_price                = 3000100;
    original.current_reference_price   = 3000050;
    original.cross_type                = 'O';
    original.price_variation_indicator = 'L';

    const auto decoded = std::get<itch::NOIIMessage>(round_trip(itch::Message {original}));
    EXPECT_EQ(decoded.paired_shares, 100000U);
    EXPECT_EQ(decoded.imbalance_shares, 2500U);
    EXPECT_EQ(decoded.imbalance_direction, 'B');
    EXPECT_EQ(itch::to_string(decoded.stock, itch::STOCK_LEN), "MSFT");
    EXPECT_EQ(decoded.near_price, 3000100U);
    EXPECT_EQ(decoded.cross_type, 'O');
}

TEST(Encoder, FrameCarriesLengthPrefix) {
    itch::SystemEventMessage event {};
    event.event_code = 'O';
    const auto frame = itch::encode_frame(itch::Message {event});
    const auto body  = itch::encode_message(itch::Message {event});
    // The 2-byte length prefix should equal the body size.
    const auto length =
        (static_cast<std::uint16_t>(frame[0]) << 8) | static_cast<std::uint16_t>(frame[1]);
    EXPECT_EQ(length, body.size());
    EXPECT_EQ(frame.size(), body.size() + 2);
}

TEST(Replay, DeliversAllMessagesAndScalesTiming) {
    // Two messages 100 ms apart in feed time; at 100x speed the wall gap is ~1 ms.
    itch::AddOrderMessage first {};
    first.stock_locate = 1;
    first.timestamp    = 0;
    fill_stock(first.stock, "AAA");
    itch::AddOrderMessage second = first;
    second.timestamp             = 100'000'000;  // 100 ms later

    std::vector<std::byte> stream;
    for (const auto& msg : {first, second}) {
        const auto frame = itch::encode_frame(itch::Message {msg});
        stream.insert(stream.end(), frame.begin(), frame.end());
    }

    itch::ReplayEngine engine {100.0};
    int                count = 0;
    const auto         start = std::chrono::steady_clock::now();
    const auto         replayed =
        engine.replay(std::span<const std::byte> {stream}, [&](const itch::Message&) { ++count; });
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(replayed, 2U);
    EXPECT_EQ(count, 2);
    // 100 ms / 100x = ~1 ms expected; assert it paced at least a little but not
    // anywhere near the full 100 ms (generous bounds to avoid flakiness).
    EXPECT_LT(elapsed, std::chrono::milliseconds {80});
}

TEST(Venue, Nasdaq50PolicyEnumeratesMessageTypes) {
    int count = 0;
    itch::venue::Nasdaq50::for_each_message_type([&count]<typename>(char) { ++count; });
    EXPECT_EQ(count, 23);  // The full ITCH 5.0 message set.
    EXPECT_EQ(itch::venue::Nasdaq50::name(), "NASDAQ TotalView-ITCH 5.0");
}
