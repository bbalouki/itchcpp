#include <gtest/gtest.h>

#include <cstring>

#include "itch/messages.hpp"
#include "itch/order_book.hpp"

namespace {

auto make_add(std::uint64_t ref, char side, std::uint32_t shares, std::uint32_t price)
    -> itch::AddOrderMessage {
    itch::AddOrderMessage msg {};
    std::memcpy(msg.stock, "ZVZZT   ", 8);
    msg.order_reference_number = ref;
    msg.buy_sell_indicator     = side;
    msg.shares                 = shares;
    msg.price                  = price;
    return msg;
}

}  // namespace

class OrderBookExtra : public ::testing::Test {
   protected:
    OrderBookExtra() : book("ZVZZT") {}
    itch::LimitOrderBook book;
};

// A crossed book (best bid >= best ask) can legitimately occur in a raw feed.
// The reconstruction must store both sides without matching or losing orders.
TEST_F(OrderBookExtra, CrossedBookKeepsBothSides) {
    book.process(make_add(1, 'B', 100, 5100));  // Aggressive bid.
    book.process(make_add(2, 'S', 100, 5000));  // Aggressive ask, below the bid.

    ASSERT_EQ(book.get_bids().size(), 1u);
    ASSERT_EQ(book.get_asks().size(), 1u);

    const std::uint32_t best_bid = book.get_bids().begin()->first;
    const std::uint32_t best_ask = book.get_asks().begin()->first;
    EXPECT_EQ(best_bid, 5100u);
    EXPECT_EQ(best_ask, 5000u);
    EXPECT_GT(best_bid, best_ask);  // The book is crossed, and that is preserved.
}

TEST_F(OrderBookExtra, BestBidIsHighestBestAskIsLowest) {
    book.process(make_add(1, 'B', 100, 4900));
    book.process(make_add(2, 'B', 100, 5000));
    book.process(make_add(3, 'B', 100, 4800));
    book.process(make_add(4, 'S', 100, 5300));
    book.process(make_add(5, 'S', 100, 5200));
    book.process(make_add(6, 'S', 100, 5400));

    EXPECT_EQ(book.get_bids().begin()->first, 5000u);  // Highest bid first.
    EXPECT_EQ(book.get_asks().begin()->first, 5200u);  // Lowest ask first.
}

// Walk a single order through its full lifecycle across message types.
TEST_F(OrderBookExtra, FullLifecycleAddExecuteReplaceCancelDelete) {
    book.process(make_add(100, 'B', 1000, 5000));
    ASSERT_EQ(book.get_bids().at(5000).total_shares, 1000u);

    // Partial execution.
    itch::OrderExecutedMessage exec {};
    exec.order_reference_number = 100;
    exec.executed_shares        = 250;
    book.process(exec);
    EXPECT_EQ(book.get_bids().at(5000).total_shares, 750u);

    // Execution with price (partial).
    itch::OrderExecutedWithPriceMessage exec_price {};
    exec_price.order_reference_number = 100;
    exec_price.executed_shares        = 250;
    exec_price.execution_price        = 5001;
    book.process(exec_price);
    EXPECT_EQ(book.get_bids().at(5000).total_shares, 500u);

    // Replace the remainder to a new price level.
    itch::OrderReplaceMessage replace {};
    replace.original_order_reference_number = 100;
    replace.new_order_reference_number      = 101;
    replace.shares                          = 500;
    replace.price                           = 5050;
    book.process(replace);
    EXPECT_EQ(book.get_bids().count(5000), 0u);
    EXPECT_EQ(book.get_bids().at(5050).total_shares, 500u);

    // Partial cancel, then delete the rest.
    itch::OrderCancelMessage cancel {};
    cancel.order_reference_number = 101;
    cancel.cancelled_shares       = 200;
    book.process(cancel);
    EXPECT_EQ(book.get_bids().at(5050).total_shares, 300u);

    itch::OrderDeleteMessage del {};
    del.order_reference_number = 101;
    book.process(del);
    EXPECT_TRUE(book.get_bids().empty());
}
