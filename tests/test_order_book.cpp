#include <gtest/gtest.h>
#include "itch/order_book.hpp"
#include "itch/messages.hpp"

TEST(LimitOrderBookTest, AddOrder) {
    itch::LimitOrderBook book;
    itch::AddOrderMessage msg;
    msg.order_reference_number = 12345;
    msg.buy_sell_indicator = 'B';
    msg.shares = 100;
    msg.price = 5000;
    book.process(msg);
    const auto& bids = book.get_bids();
    ASSERT_EQ(bids.size(), 1);
    ASSERT_EQ(bids.begin()->first, 5000);
    ASSERT_EQ(bids.begin()->second.total_shares, 100);
}

TEST(LimitOrderBookTest, ExecuteOrder) {
    itch::LimitOrderBook book;
    itch::AddOrderMessage add_msg;
    add_msg.order_reference_number = 12345;
    add_msg.buy_sell_indicator = 'S';
    add_msg.shares = 100;
    add_msg.price = 5000;
    book.process(add_msg);

    itch::OrderExecutedMessage exec_msg;
    exec_msg.order_reference_number = 12345;
    exec_msg.executed_shares = 50;
    book.process(exec_msg);

    const auto& asks = book.get_asks();
    ASSERT_EQ(asks.size(), 1);
    ASSERT_EQ(asks.begin()->second.total_shares, 50);
}

TEST(LimitOrderBookTest, DeleteOrder) {
    itch::LimitOrderBook book;
    itch::AddOrderMessage add_msg;
    add_msg.order_reference_number = 12345;
    add_msg.buy_sell_indicator = 'B';
    add_msg.shares = 100;
    add_msg.price = 5000;
    book.process(add_msg);

    itch::OrderDeleteMessage del_msg;
    del_msg.order_reference_number = 12345;
    book.process(del_msg);

    const auto& bids = book.get_bids();
    ASSERT_TRUE(bids.empty());
}

TEST(LimitOrderBookTest, ReplaceOrder) {
    itch::LimitOrderBook book;
    itch::AddOrderMessage add_msg;
    add_msg.order_reference_number = 12345;
    add_msg.buy_sell_indicator = 'B';
    add_msg.shares = 100;
    add_msg.price = 5000;
    book.process(add_msg);

    itch::OrderReplaceMessage repl_msg;
    repl_msg.original_order_reference_number = 12345;
    repl_msg.new_order_reference_number = 54321;
    repl_msg.shares = 200;
    repl_msg.price = 5001;
    book.process(repl_msg);

    const auto& bids = book.get_bids();
    ASSERT_EQ(bids.size(), 1);
    ASSERT_EQ(bids.begin()->first, 5001);
    ASSERT_EQ(bids.begin()->second.total_shares, 200);
}

TEST(LimitOrderBookTest, CancelNonExistentOrder) {
    itch::LimitOrderBook book;
    itch::OrderCancelMessage msg;
    msg.order_reference_number = 12345;
    msg.cancelled_shares = 100;
    book.process(msg);
    ASSERT_TRUE(book.get_bids().empty());
    ASSERT_TRUE(book.get_asks().empty());
}

TEST(LimitOrderBookTest, DeleteNonExistentOrder) {
    itch::LimitOrderBook book;
    itch::OrderDeleteMessage msg;
    msg.order_reference_number = 12345;
    book.process(msg);
    ASSERT_TRUE(book.get_bids().empty());
    ASSERT_TRUE(book.get_asks().empty());
}

TEST(LimitOrderBookTest, ReplaceNonExistentOrder) {
    itch::LimitOrderBook book;
    itch::OrderReplaceMessage msg;
    msg.original_order_reference_number = 12345;
    msg.new_order_reference_number = 54321;
    msg.shares = 100;
    msg.price = 5000;
    book.process(msg);
    ASSERT_TRUE(book.get_bids().empty());
    ASSERT_TRUE(book.get_asks().empty());
}
