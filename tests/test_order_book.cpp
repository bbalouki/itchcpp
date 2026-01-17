#include <gtest/gtest.h>

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "itch/messages.hpp"
#include "itch/order_book.hpp"

void set_stock(char* dest, const std::string& src) { std::strncpy(dest, src.c_str(), 8); }

void set_stock_spaces(char* dest, const std::string& src) {
    std::memset(dest, ' ', 8);
    std::memcpy(dest, src.data(), std::min(src.size(), size_t(8)));
}

class LimitOrderBookTest : public ::testing::Test {
   protected:
    LimitOrderBookTest() : book("ZVZZT") {}
    itch::LimitOrderBook book;
};

TEST_F(LimitOrderBookTest, AddOrder_Symbol_NullPadded) {
    itch::AddOrderMessage msg {};
    set_stock(msg.stock, "ZVZZT");
    msg.order_reference_number = 1;
    msg.buy_sell_indicator     = 'B';
    msg.shares                 = 10;
    msg.price                  = 100;
    book.process(msg);
    ASSERT_EQ(book.get_bids().size(), 1);
}

TEST_F(LimitOrderBookTest, AddOrder_Symbol_SpacePadded) {
    itch::AddOrderMessage msg {};
    set_stock_spaces(msg.stock, "ZVZZT");
    msg.order_reference_number = 2;
    msg.buy_sell_indicator     = 'B';
    msg.shares                 = 10;
    msg.price                  = 100;
    book.process(msg);
    ASSERT_EQ(book.get_bids().size(), 1);
}

TEST_F(LimitOrderBookTest, AddOrder_Symbol_FullLength) {
    itch::LimitOrderBook  longBook("ZVZZTXYW");
    itch::AddOrderMessage msg {};
    std::memcpy(msg.stock, "ZVZZTXYW", 8);
    msg.order_reference_number = 3;
    msg.buy_sell_indicator     = 'B';
    msg.shares                 = 10;
    msg.price                  = 100;

    longBook.process(msg);
    ASSERT_EQ(longBook.get_bids().size(), 1);
}

TEST_F(LimitOrderBookTest, AddOrder_Symbol_Mismatch_SpaceVsNull) {
    itch::AddOrderMessage msg {};
    set_stock(msg.stock, "ZVZZT1");
    msg.order_reference_number = 4;
    book.process(msg);
    ASSERT_TRUE(book.get_bids().empty());
}

TEST_F(LimitOrderBookTest, Bid_Partial_Then_Full_Execution) {
    itch::AddOrderMessage add {};
    set_stock(add.stock, "ZVZZT");
    add.order_reference_number = 100;
    add.buy_sell_indicator     = 'B';
    add.shares                 = 1000;
    add.price                  = 5000;
    book.process(add);

    // Partial
    itch::OrderExecutedMessage exec1 {};
    exec1.order_reference_number = 100;
    exec1.executed_shares        = 300;
    book.process(exec1);
    ASSERT_EQ(book.get_bids().at(5000).total_shares, 700);

    // Full
    itch::OrderExecutedMessage exec2 {};
    exec2.order_reference_number = 100;
    exec2.executed_shares        = 700;
    book.process(exec2);
    ASSERT_TRUE(book.get_bids().empty());
}

TEST_F(LimitOrderBookTest, Ask_Partial_Then_Full_Cancel) {
    itch::AddOrderMessage add {};
    set_stock(add.stock, "ZVZZT");
    add.order_reference_number = 200;
    add.buy_sell_indicator     = 'S';
    add.shares                 = 1000;
    add.price                  = 6000;
    book.process(add);

    // Partial
    itch::OrderCancelMessage can1 {};
    can1.order_reference_number = 200;
    can1.cancelled_shares       = 300;
    book.process(can1);
    ASSERT_EQ(book.get_asks().at(6000).total_shares, 700);

    // Full
    itch::OrderCancelMessage can2 {};
    can2.order_reference_number = 200;
    can2.cancelled_shares       = 700;
    book.process(can2);
    ASSERT_TRUE(book.get_asks().empty());
}

TEST_F(LimitOrderBookTest, Multiple_Orders_One_Level_Delete_Middle) {
    for (int i = 1; i <= 3; ++i) {
        itch::AddOrderMessage add {};
        set_stock(add.stock, "ZVZZT");
        add.order_reference_number = i;
        add.buy_sell_indicator     = 'B';
        add.shares                 = 100;
        add.price                  = 5000;
        book.process(add);
    }
    ASSERT_EQ(book.get_bids().at(5000).total_shares, 300);

    // Delete Middle (Ref 2)
    itch::OrderDeleteMessage del {};
    del.order_reference_number = 2;
    book.process(del);

    ASSERT_EQ(book.get_bids().at(5000).total_shares, 200);
    ASSERT_EQ(book.get_bids().size(), 1);
}

TEST_F(LimitOrderBookTest, Replace_PriceChange_SameLevelCreation) {
    // Add Order
    itch::AddOrderMessage add {};
    set_stock(add.stock, "ZVZZT");
    add.order_reference_number = 10;
    add.buy_sell_indicator     = 'B';
    add.shares                 = 100;
    add.price                  = 5000;
    book.process(add);

    // Replace to NEW price
    itch::OrderReplaceMessage rep {};
    rep.original_order_reference_number = 10;
    rep.new_order_reference_number      = 11;
    rep.shares                          = 100;
    rep.price                           = 5100;
    book.process(rep);

    ASSERT_EQ(book.get_bids().count(5000), 0);
    ASSERT_EQ(book.get_bids().count(5100), 1);
}

TEST_F(LimitOrderBookTest, Replace_Same_ID) {
    itch::AddOrderMessage add {};
    set_stock(add.stock, "ZVZZT");
    add.order_reference_number = 20;
    add.buy_sell_indicator     = 'S';
    add.shares                 = 100;
    add.price                  = 6000;
    book.process(add);

    itch::OrderReplaceMessage rep {};
    rep.original_order_reference_number = 20;
    rep.new_order_reference_number      = 20;    // Same ID
    rep.shares                          = 150;   // Change shares
    rep.price                           = 6000;  // Same price
    book.process(rep);

    ASSERT_EQ(book.get_asks().at(6000).total_shares, 150);
}

TEST_F(LimitOrderBookTest, Replace_NonExistent) {
    itch::OrderReplaceMessage rep {};
    rep.original_order_reference_number = 999;
    rep.new_order_reference_number      = 888;
    book.process(rep);
    ASSERT_TRUE(book.get_bids().empty());
}

TEST_F(LimitOrderBookTest, AddOrder_Attribution_Success) {
    itch::AddOrderMPIDAttributionMessage msg {};
    set_stock(msg.stock, "ZVZZT");
    msg.order_reference_number = 50;
    msg.buy_sell_indicator     = 'B';
    msg.shares                 = 50;
    msg.price                  = 5000;
    book.process(msg);
    ASSERT_EQ(book.get_bids().size(), 1);
}

TEST_F(LimitOrderBookTest, AddOrder_Attribution_WrongStock) {
    itch::AddOrderMPIDAttributionMessage msg {};
    set_stock(msg.stock, "WRONG");
    msg.order_reference_number = 51;
    book.process(msg);
    ASSERT_TRUE(book.get_bids().empty());
}

TEST_F(LimitOrderBookTest, ExecWithPrice_Success) {
    itch::AddOrderMessage add {};
    set_stock(add.stock, "ZVZZT");
    add.order_reference_number = 60;
    add.buy_sell_indicator     = 'S';
    add.shares                 = 100;
    add.price                  = 5500;
    book.process(add);

    itch::OrderExecutedWithPriceMessage exec {};
    exec.order_reference_number = 60;
    exec.executed_shares        = 100;
    exec.execution_price        = 5505;
    book.process(exec);
    ASSERT_TRUE(book.get_asks().empty());
}

TEST_F(LimitOrderBookTest, ExecWithPrice_Unknown) {
    itch::OrderExecutedWithPriceMessage exec {};
    exec.order_reference_number = 9999;
    book.process(exec);
    // Should pass without crash
}

TEST_F(LimitOrderBookTest, Print_Fully_Empty) {
    std::stringstream stream;
    book.print(stream);
    std::string out = stream.str();
    EXPECT_NE(out.find("SHARES"), std::string::npos);
    EXPECT_EQ(out.find("Bid"), std::string::npos);
}

TEST_F(LimitOrderBookTest, Print_With_Delay) {
    itch::AddOrderMessage b {};
    set_stock(b.stock, "ZVZZT");
    b.order_reference_number = 1;
    b.buy_sell_indicator     = 'B';
    b.shares                 = 10;
    b.price                  = 100;
    book.process(b);

    itch::AddOrderMessage a {};
    set_stock(a.stock, "ZVZZT");
    a.order_reference_number = 2;
    a.buy_sell_indicator     = 'S';
    a.shares                 = 10;
    a.price                  = 200;
    book.process(a);

    std::stringstream stream;
    book.print(stream, 1);
    std::string out = stream.str();
    EXPECT_NE(out.find("Bid"), std::string::npos);
    EXPECT_NE(out.find("Ask"), std::string::npos);
}

TEST_F(LimitOrderBookTest, Process_All_Message_Types) {
    // 1. System Messages
    book.process(itch::SystemEventMessage {});
    book.process(itch::StockDirectoryMessage {});
    book.process(itch::StockTradingActionMessage {});
    book.process(itch::RegSHOMessage {});
    book.process(itch::MarketParticipantPositionMessage {});
    book.process(itch::MWCBDeclineLevelMessage {});
    book.process(itch::MWCBStatusMessage {});
    book.process(itch::IPOQuotingPeriodUpdateMessage {});
    book.process(itch::LULDAuctionCollarMessage {});
    book.process(itch::OperationalHaltMessage {});

    // 2. Book Messages (Handled)
    book.process(itch::AddOrderMessage {});
    book.process(itch::AddOrderMPIDAttributionMessage {});
    book.process(itch::OrderExecutedMessage {});
    book.process(itch::OrderExecutedWithPriceMessage {});
    book.process(itch::OrderCancelMessage {});
    book.process(itch::OrderDeleteMessage {});
    book.process(itch::OrderReplaceMessage {});

    // 3. Trade/Indicator Messages
    book.process(itch::NonCrossTradeMessage {});
    book.process(itch::CrossTradeMessage {});
    book.process(itch::BrokenTradeMessage {});
    book.process(itch::NOIIMessage {});
    book.process(itch::RetailPriceImprovementIndicatorMessage {});
    book.process(itch::DLCRMessage {});

    ASSERT_TRUE(book.get_bids().empty());
}
