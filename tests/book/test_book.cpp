#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "itch/book/book_manager.hpp"
#include "itch/book/l3_book.hpp"
#include "itch/messages.hpp"

namespace {

using itch::book::L3Book;
using itch::book::Side;

// Fills an 8-byte ITCH stock field from a symbol, space padded.
auto fill_stock(char (&dest)[itch::STOCK_LEN], std::string_view symbol) -> void {
    std::memset(dest, ' ', itch::STOCK_LEN);
    std::memcpy(
        dest, symbol.data(), std::min(symbol.size(), static_cast<std::size_t>(itch::STOCK_LEN))
    );
}

auto make_add(
    std::uint16_t    locate,
    std::uint64_t    ref,
    char             side,
    std::uint32_t    shares,
    std::string_view symbol,
    std::uint32_t    price
) -> itch::AddOrderMessage {
    itch::AddOrderMessage msg {};
    msg.stock_locate           = locate;
    msg.order_reference_number = ref;
    msg.buy_sell_indicator     = side;
    msg.shares                 = shares;
    fill_stock(msg.stock, symbol);
    msg.price = price;
    return msg;
}

}  // namespace

TEST(L3Book, TracksBestBidOfferAcrossLevels) {
    L3Book book {"AAPL"};
    book.add_order(1, Side::buy, 100, 1500000);
    book.add_order(2, Side::buy, 200, 1500100);  // better bid
    book.add_order(3, Side::sell, 50, 1500300);
    book.add_order(4, Side::sell, 75, 1500200);  // better ask

    const auto bbo = book.bbo();
    ASSERT_TRUE(bbo.has_bid);
    ASSERT_TRUE(bbo.has_ask);
    EXPECT_EQ(bbo.bid_price.raw(), 1500100U);
    EXPECT_EQ(bbo.bid_shares, 200U);
    EXPECT_EQ(bbo.ask_price.raw(), 1500200U);
    EXPECT_EQ(bbo.ask_shares, 75U);
}

TEST(L3Book, AggregatesSharesAndKeepsTimePriority) {
    L3Book book;
    book.add_order(1, Side::buy, 100, 1000000);
    book.add_order(2, Side::buy, 150, 1000000);  // same level, later in queue
    const auto depth = book.depth(Side::buy);
    ASSERT_EQ(depth.size(), 1U);
    EXPECT_EQ(depth.front().shares, 250U);
    EXPECT_EQ(depth.front().order_count, 2U);

    const auto orders = book.orders_at(Side::buy, 1000000);
    ASSERT_EQ(orders.size(), 2U);
    EXPECT_EQ(orders[0].reference_number, 1U);  // FIFO order preserved
    EXPECT_EQ(orders[1].reference_number, 2U);
}

TEST(L3Book, ExecutePartialThenFullRemovesOrder) {
    L3Book book;
    book.add_order(1, Side::sell, 100, 2000000);
    EXPECT_EQ(book.execute_order(1, 40), 40U);
    EXPECT_EQ(book.bbo().ask_shares, 60U);
    EXPECT_EQ(book.execute_order(1, 999), 60U);  // clamps to remaining
    EXPECT_FALSE(book.contains(1));
    EXPECT_FALSE(book.bbo().has_ask);
}

TEST(L3Book, CancelReducesAndDeleteRemoves) {
    L3Book book;
    book.add_order(1, Side::buy, 500, 1000000);
    book.reduce_order(1, 200);
    EXPECT_EQ(book.bbo().bid_shares, 300U);
    book.delete_order(1);
    EXPECT_TRUE(book.empty());
}

TEST(L3Book, ReplaceMovesOrderToNewPriceAndReference) {
    L3Book book;
    book.add_order(1, Side::buy, 100, 1000000);
    book.replace_order(1, 2, 80, 1000500);
    EXPECT_FALSE(book.contains(1));
    ASSERT_TRUE(book.contains(2));
    EXPECT_EQ(book.bbo().bid_price.raw(), 1000500U);
    EXPECT_EQ(book.bbo().bid_shares, 80U);
}

TEST(L3Book, HandlesCrossedBookState) {
    L3Book book;
    book.add_order(1, Side::buy, 100, 1000500);   // bid above
    book.add_order(2, Side::sell, 100, 1000000);  // ask below -> crossed
    const auto bbo = book.bbo();
    EXPECT_GT(bbo.bid_price.raw(), bbo.ask_price.raw());  // book represents the crossed state
}

TEST(BookManager, RoutesMessagesToPerSymbolBooks) {
    itch::book::BookManager manager;
    manager.process(itch::Message {make_add(1, 10, 'B', 100, "AAPL", 1500000)});
    manager.process(itch::Message {make_add(2, 11, 'S', 200, "MSFT", 3000000)});

    EXPECT_EQ(manager.book_count(), 2U);
    const auto* apple = manager.book_for_symbol("AAPL");
    ASSERT_NE(apple, nullptr);
    EXPECT_EQ(apple->bbo().bid_shares, 100U);
    EXPECT_EQ(manager.book(2)->symbol(), "MSFT");
}

TEST(BookManager, UniverseFilterTracksOnlySelectedSymbols) {
    itch::book::BookManager manager;
    manager.track_symbol("AAPL");
    manager.process(itch::Message {make_add(1, 10, 'B', 100, "AAPL", 1500000)});
    manager.process(itch::Message {make_add(2, 11, 'S', 200, "MSFT", 3000000)});

    EXPECT_EQ(manager.book_count(), 1U);
    EXPECT_NE(manager.book_for_symbol("AAPL"), nullptr);
    EXPECT_EQ(manager.book_for_symbol("MSFT"), nullptr);
}

TEST(BookManager, EmitsBboEventsOnlyWhenTopChanges) {
    itch::book::BookManager manager;
    int                     bbo_events = 0;
    manager.set_bbo_callback([&](const L3Book&, const itch::book::Bbo&) { ++bbo_events; });

    manager.process(itch::Message {make_add(1, 10, 'B', 100, "AAPL", 1500000)});  // new top
    manager.process(itch::Message {make_add(1, 11, 'B', 100, "AAPL", 1400000)}
    );  // worse, no change
    EXPECT_EQ(bbo_events, 1);
}

TEST(BookManager, ExtractsTradeTapeWithPrintableFlag) {
    itch::book::BookManager  manager;
    std::vector<itch::Trade> tape;
    manager.set_trade_callback([&](const itch::Trade& trade) { tape.push_back(trade); });

    manager.process(itch::Message {make_add(1, 10, 'S', 100, "AAPL", 1500000)});

    itch::OrderExecutedMessage exec {};
    exec.stock_locate           = 1;
    exec.order_reference_number = 10;
    exec.executed_shares        = 40;
    exec.match_number           = 999;
    manager.process(itch::Message {exec});

    itch::OrderExecutedWithPriceMessage exec_price {};
    exec_price.stock_locate           = 1;
    exec_price.order_reference_number = 10;
    exec_price.executed_shares        = 10;
    exec_price.printable              = 'N';
    exec_price.execution_price        = 1499000;
    manager.process(itch::Message {exec_price});

    ASSERT_EQ(tape.size(), 2U);
    EXPECT_EQ(tape[0].price.raw(), 1500000U);  // E uses the resting order price
    EXPECT_TRUE(tape[0].printable);
    EXPECT_EQ(tape[1].price.raw(), 1499000U);  // C uses the execution price
    EXPECT_FALSE(tape[1].printable);           // honours the printable flag
}
