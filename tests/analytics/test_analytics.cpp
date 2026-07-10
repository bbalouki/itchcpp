#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <vector>

#include "itch/analytics/auctions.hpp"
#include "itch/analytics/bars.hpp"
#include "itch/analytics/imbalance.hpp"
#include "itch/analytics/microstructure.hpp"
#include "itch/analytics/vwap.hpp"
#include "itch/book/l3_book.hpp"
#include "itch/messages.hpp"
#include "itch/tape.hpp"

namespace {

auto make_trade(std::uint64_t timestamp, std::uint32_t raw_price, std::uint64_t shares)
    -> itch::Trade {
    itch::Trade trade {};
    trade.timestamp = timestamp;
    trade.price     = itch::StandardPrice {raw_price};
    trade.shares    = shares;
    return trade;
}

auto fill_stock(char (&dest)[itch::STOCK_LEN], std::string_view symbol) -> void {
    std::memset(dest, ' ', itch::STOCK_LEN);
    std::memcpy(
        dest, symbol.data(), std::min(symbol.size(), static_cast<std::size_t>(itch::STOCK_LEN))
    );
}

}  // namespace

TEST(Bars, TimeClockGroupsTradesIntoIntervals) {
    std::vector<itch::analytics::Bar> bars;
    itch::analytics::BarBuilder       builder {
        itch::analytics::TimeClock {10},
        [&](const itch::analytics::Bar& bar) { bars.push_back(bar); }
    };

    builder.add(make_trade(1, 100, 5));  // bucket 0
    builder.add(make_trade(5, 120, 3));  // bucket 0
    builder.add(make_trade(12, 90, 4));  // bucket 1 -> closes bucket 0
    builder.flush();

    ASSERT_EQ(bars.size(), 2U);
    EXPECT_EQ(bars[0].open.raw(), 100U);
    EXPECT_EQ(bars[0].high.raw(), 120U);
    EXPECT_EQ(bars[0].low.raw(), 100U);
    EXPECT_EQ(bars[0].close.raw(), 120U);
    EXPECT_EQ(bars[0].volume, 8U);
    EXPECT_EQ(bars[0].trade_count, 2U);
    EXPECT_EQ(bars[1].open.raw(), 90U);
    EXPECT_EQ(bars[1].volume, 4U);
}

TEST(Bars, VolumeClockClosesBarsOnVolumeThreshold) {
    std::vector<itch::analytics::Bar> bars;
    itch::analytics::BarBuilder       builder {
        itch::analytics::VolumeClock {100},
        [&](const itch::analytics::Bar& bar) { bars.push_back(bar); }
    };
    builder.add(make_trade(1, 100, 60));  // cumulative 0  -> bucket 0
    builder.add(make_trade(2, 101, 50));  // cumulative 60 -> bucket 0 (now 110)
    builder.add(make_trade(3, 102, 10));  // cumulative 110 -> bucket 1 -> closes bar 0
    builder.flush();
    ASSERT_EQ(bars.size(), 2U);
    EXPECT_EQ(bars[0].volume, 110U);
}

TEST(Vwap, ComputesVolumeWeightedAverage) {
    itch::analytics::Vwap vwap;
    vwap.add(itch::StandardPrice {100}, 10);
    vwap.add(itch::StandardPrice {200}, 30);
    // (100*10 + 200*30) / 40 = 175 raw -> /10000 = 0.0175
    EXPECT_DOUBLE_EQ(vwap.value(), 175.0 / 10000.0);
    EXPECT_EQ(vwap.volume(), 40U);
}

TEST(Twap, ComputesTimeWeightedAverage) {
    itch::analytics::Twap twap;
    twap.add(itch::StandardPrice {10000}, 0);   // price 1.0 from t=0
    twap.add(itch::StandardPrice {30000}, 10);  // 1.0 held for 10 units, then 3.0
    twap.add(itch::StandardPrice {30000}, 20);  // 3.0 held for 10 units
    // (1.0*10 + 3.0*10) / 20 = 2.0
    EXPECT_DOUBLE_EQ(twap.value(), 2.0);
}

TEST(Microstructure, SpreadMidAndQueueImbalance) {
    itch::book::Bbo bbo {};
    bbo.has_bid    = true;
    bbo.has_ask    = true;
    bbo.bid_price  = itch::StandardPrice {1000000};
    bbo.ask_price  = itch::StandardPrice {1000200};
    bbo.bid_shares = 300;
    bbo.ask_shares = 100;

    EXPECT_NEAR(itch::analytics::spread(bbo), 0.02, 1e-9);
    EXPECT_NEAR(itch::analytics::mid_price(bbo), 100.01, 1e-9);
    EXPECT_DOUBLE_EQ(itch::analytics::queue_imbalance(bbo), 0.5);  // (300-100)/400
}

TEST(Microstructure, OrderFlowImbalanceFollowsContModel) {
    itch::book::Bbo previous {};
    previous.bid_price  = itch::StandardPrice {1000000};
    previous.ask_price  = itch::StandardPrice {1000200};
    previous.bid_shares = 100;
    previous.ask_shares = 100;

    itch::book::Bbo current = previous;
    current.bid_shares      = 150;  // size added at unchanged best bid -> +50 buy flow
    EXPECT_DOUBLE_EQ(itch::analytics::order_flow_imbalance(previous, current), 50.0);
}

TEST(Microstructure, DepthAtLevelSumsBestLevels) {
    itch::book::L3Book book;
    book.add_order(1, itch::book::Side::buy, 100, 1000000);
    book.add_order(2, itch::book::Side::buy, 200, 999900);
    book.add_order(3, itch::book::Side::buy, 300, 999800);
    EXPECT_EQ(itch::analytics::depth_at_level(book, itch::book::Side::buy, 2), 300U);
    EXPECT_EQ(itch::analytics::depth_at_level(book, itch::book::Side::buy, 0), 600U);
}

TEST(Imbalance, SurfacesNoiiFields) {
    itch::NOIIMessage msg {};
    msg.stock_locate = 5;
    msg.timestamp    = 42;
    fill_stock(msg.stock, "AAPL");
    msg.paired_shares           = 1000;
    msg.imbalance_shares        = 250;
    msg.imbalance_direction     = 'B';
    msg.current_reference_price = 1500000;

    const auto info = itch::analytics::make_imbalance_info(msg);
    EXPECT_EQ(info.stock, "AAPL");
    EXPECT_EQ(info.paired_shares, 1000U);
    EXPECT_EQ(info.imbalance_shares, 250U);
    EXPECT_EQ(info.current_reference_price.raw(), 1500000U);
    EXPECT_EQ(itch::analytics::imbalance_direction_name('B'), "Buy imbalance");
}

TEST(Auctions, ReconstructsCrossWithImbalanceContext) {
    itch::analytics::AuctionTracker       tracker;
    std::vector<itch::analytics::Auction> auctions;
    tracker.set_auction_callback([&](const itch::analytics::Auction& auction) {
        auctions.push_back(auction);
    });

    itch::NOIIMessage noii {};
    noii.stock_locate = 7;
    fill_stock(noii.stock, "MSFT");
    noii.paired_shares       = 5000;
    noii.imbalance_shares    = 1200;
    noii.imbalance_direction = 'S';
    tracker.process(itch::Message {noii});

    itch::CrossTradeMessage cross {};
    cross.stock_locate = 7;
    fill_stock(cross.stock, "MSFT");
    cross.shares      = 6200;
    cross.cross_price = 3000000;
    cross.cross_type  = 'O';
    tracker.process(itch::Message {cross});

    ASSERT_EQ(auctions.size(), 1U);
    EXPECT_EQ(auctions[0].cross_price.raw(), 3000000U);
    EXPECT_EQ(auctions[0].cross_type, 'O');
    EXPECT_TRUE(auctions[0].had_imbalance);
    EXPECT_EQ(auctions[0].imbalance_shares, 1200U);
    EXPECT_EQ(itch::analytics::cross_type_name('O'), "Opening Cross");
}
