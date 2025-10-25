#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "parser.hpp"

TEST(MessagesTest, StockDirectoryMessage) {
    const char raw_msg[] = {'\x00', '\x27', 'R',    '\x00', '\x04', '\x00', '\x05', '\x00', '\x00',
                            '\x00', '\x00', '\x00', '\x06', 'S',    'T',    'O',    'C',    'K',
                            '1',    ' ',    ' ',    'C',    'N',    '\x00', '\x00', '\x00', '\x64',
                            'N',    'C',    ' ',    ' ',    'P',    'N',    'N',    ' ',    'N',
                            '\x00', '\x00', '\x00', '\x00', 'N'};

    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::StockDirectoryMessage>(messages[0]));
    auto msg = std::get<itch::StockDirectoryMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 4);
    EXPECT_EQ(msg.tracking_number, 5);
    EXPECT_EQ(msg.timestamp, 6);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.market_category, 'C');
    EXPECT_EQ(msg.financial_status_indicator, 'N');
    EXPECT_EQ(msg.round_lot_size, 100);
    EXPECT_EQ(msg.round_lots_only, 'N');
    EXPECT_EQ(msg.issue_classification, 'C');
    EXPECT_EQ(std::string(msg.issue_sub_type, 2), "  ");
    EXPECT_EQ(msg.authenticity, 'P');
    EXPECT_EQ(msg.short_sale_threshold_indicator, 'N');
    EXPECT_EQ(msg.ipo_flag, 'N');
    EXPECT_EQ(msg.luld_ref, ' ');
    EXPECT_EQ(msg.etp_flag, 'N');
    EXPECT_EQ(msg.etp_leverage_factor, 0);
    EXPECT_EQ(msg.inverse_indicator, 'N');
}

TEST(MessagesTest, StockTradingActionMessage) {
    const char raw_msg[] = {'\x00', '\x19', 'H',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
                            '\x00', '\x00', '\x00', '\x03', 'S',    'T',    'O',    'C',    'K',
                            '1',    ' ',    ' ',    'T',    ' ',    'R',    'S',    'N',    ' '};

    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::StockTradingActionMessage>(messages[0]));
    auto msg = std::get<itch::StockTradingActionMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.trading_state, 'T');
    EXPECT_EQ(msg.reserved, ' ');
    EXPECT_EQ(std::string(msg.reason, 4), "RSN ");
}

TEST(MessagesTest, RegSHOMessage) {
    const char        raw_msg[] = {'\x00', '\x14', 'Y',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', 'S',    'T',    'O',
                                   'C',    'K',    '1',    ' ',    ' ',    'A'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::RegSHOMessage>(messages[0]));
    auto msg = std::get<itch::RegSHOMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.reg_sho_action, 'A');
}

TEST(MessagesTest, MarketParticipantPositionMessage) {
    const char raw_msg[] = {'\x00', '\x1a', 'L',    '\x00', '\x01', '\x00', '\x02',
                            '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', 'M',
                            'P',    'I',    'D',    'S',    'T',    'O',    'C',
                            'K',    '1',    ' ',    ' ',    'Y',    'N',    'A'};

    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::MarketParticipantPositionMessage>(messages[0]));
    auto msg = std::get<itch::MarketParticipantPositionMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.mpid, 4), "MPID");
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.primary_market_maker, 'Y');
    EXPECT_EQ(msg.market_maker_mode, 'N');
    EXPECT_EQ(msg.market_participant_state, 'A');
}

TEST(MessagesTest, MWCBDeclineLevelMessage) {
    const char raw_msg[] = {
        '\x00', '\x23', 'V',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00', '\x00',
        '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x04', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x05', '\x00',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x06',
    };
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::MWCBDeclineLevelMessage>(messages[0]));
    auto msg = std::get<itch::MWCBDeclineLevelMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.level1, 4);
    EXPECT_EQ(msg.level2, 5);
    EXPECT_EQ(msg.level3, 6);
}

TEST(MessagesTest, MWCBStatusMessage) {
    const char        raw_msg[] = {'\x00', '\x0c', 'W',    '\x00', '\x01', '\x00', '\x02',
                                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', 'B'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::MWCBStatusMessage>(messages[0]));
    auto msg = std::get<itch::MWCBStatusMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.breached_level, 'B');
}

TEST(MessagesTest, IPOQuotingPeriodUpdateMessage) {
    const char        raw_msg[] = {'\x00', '\x1c', 'K',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', 'S',    'T',    'O',
                                   'C',    'K',    '1',    ' ',    ' ',    '\x00', '\x00', '\x00',
                                   '\x04', 'Q',    '\x00', '\x00', '\x00', '\x05'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::IPOQuotingPeriodUpdateMessage>(messages[0]));
    auto msg = std::get<itch::IPOQuotingPeriodUpdateMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.ipo_quotation_release_time, 4);
    EXPECT_EQ(msg.ipo_quotation_release_qualifier, 'Q');
    EXPECT_EQ(msg.ipo_price, 5);
}

TEST(MessagesTest, LULDAuctionCollarMessage) {
    const char        raw_msg[] = {'\x00', '\x23', 'J',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', 'S',    'T',    'O',
                                   'C',    'K',    '1',    ' ',    ' ',    '\x00', '\x00', '\x00',
                                   '\x04', '\x00', '\x00', '\x00', '\x05', '\x00', '\x00', '\x00',
                                   '\x06', '\x00', '\x00', '\x00', '\x07'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::LULDAuctionCollarMessage>(messages[0]));
    auto msg = std::get<itch::LULDAuctionCollarMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.auction_collar_reference_price, 4);
    EXPECT_EQ(msg.upper_auction_collar_price, 5);
    EXPECT_EQ(msg.lower_auction_collar_price, 6);
    EXPECT_EQ(msg.auction_collar_extension, 7);
}

TEST(MessagesTest, OperationalHaltMessage) {
    const char        raw_msg[] = {'\x00', '\x15', 'h',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', 'S',    'T',    'O',
                                   'C',    'K',    '1',    ' ',    ' ',    'N',    'H'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::OperationalHaltMessage>(messages[0]));
    auto msg = std::get<itch::OperationalHaltMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.market_code, 'N');
    EXPECT_EQ(msg.operational_halt_action, 'H');
}

TEST(MessagesTest, AddOrderMessage) {
    const char        raw_msg[] = {'\x00', '\x24', 'A',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x04', 'B',    '\x00', '\x00',
                                   '\x00', '\x64', 'S',    'T',    'O',    'C',    'K',    '1',
                                   ' ',    ' ',    '\x00', '\x00', '\x00', '\x05'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::AddOrderMessage>(messages[0]));
    auto msg = std::get<itch::AddOrderMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
    EXPECT_EQ(msg.buy_sell_indicator, 'B');
    EXPECT_EQ(msg.shares, 100);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.price, 5);
}

TEST(MessagesTest, AddOrderMPIDAttributionMessage) {
    const char  raw_msg[] = {'\x00', '\x28', 'F',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00',
                             '\x00', '\x00', '\x04', 'B',    '\x00', '\x00', '\x00', '\x64', 'S',
                             'T',    'O',    'C',    'K',    '1',    ' ',    ' ',    '\x00', '\x00',
                             '\x00', '\x05', 'A',    'T',    'T',    'R'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::AddOrderMPIDAttributionMessage>(messages[0]));
    auto msg = std::get<itch::AddOrderMPIDAttributionMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
    EXPECT_EQ(msg.buy_sell_indicator, 'B');
    EXPECT_EQ(msg.shares, 100);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.price, 5);
    EXPECT_EQ(std::string(msg.attribution, 4), "ATTR");
}

TEST(MessagesTest, OrderExecutedMessage) {
    const char  raw_msg[] = {'\x00', '\x1f', 'E',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00',
                             '\x00', '\x00', '\x04', '\x00', '\x00', '\x00', '\x64', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x00', '\x00', '\x05'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::OrderExecutedMessage>(messages[0]));
    auto msg = std::get<itch::OrderExecutedMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
    EXPECT_EQ(msg.executed_shares, 100);
    EXPECT_EQ(msg.match_number, 5);
}

TEST(MessagesTest, OrderExecutedWithPriceMessage) {
    const char        raw_msg[] = {'\x00', '\x24', 'C',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x04', '\x00', '\x00', '\x00',
                                   '\x64', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                                   '\x05', 'Y',    '\x00', '\x00', '\x00', '\x06'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::OrderExecutedWithPriceMessage>(messages[0]));
    auto msg = std::get<itch::OrderExecutedWithPriceMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
    EXPECT_EQ(msg.executed_shares, 100);
    EXPECT_EQ(msg.match_number, 5);
    EXPECT_EQ(msg.printable, 'Y');
    EXPECT_EQ(msg.execution_price, 6);
}

TEST(MessagesTest, OrderCancelMessage) {
    const char  raw_msg[] = {'\x00', '\x17', 'X',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00',
                             '\x00', '\x00', '\x04', '\x00', '\x00', '\x00', '\x64'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::OrderCancelMessage>(messages[0]));
    auto msg = std::get<itch::OrderCancelMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
    EXPECT_EQ(msg.cancelled_shares, 100);
}

TEST(MessagesTest, OrderDeleteMessage) {
    const char        raw_msg[] = {'\x00', '\x13', 'D',    '\x00', '\x01', '\x00', '\x02',
                                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x04'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::OrderDeleteMessage>(messages[0]));
    auto msg = std::get<itch::OrderDeleteMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
}

TEST(MessagesTest, OrderReplaceMessage) {
    const char        raw_msg[] = {'\x00', '\x23', 'U',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x04', '\x00', '\x00', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x05', '\x00', '\x00', '\x00',
                                   '\x64', '\x00', '\x00', '\x00', '\x06'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::OrderReplaceMessage>(messages[0]));
    auto msg = std::get<itch::OrderReplaceMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.original_order_reference_number, 4);
    EXPECT_EQ(msg.new_order_reference_number, 5);
    EXPECT_EQ(msg.shares, 100);
    EXPECT_EQ(msg.price, 6);
}

TEST(MessagesTest, NonCrossTradeMessage) {
    const char        raw_msg[] = {'\x00', '\x2c', 'P',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x04', 'B',    '\x00', '\x00',
                                   '\x00', '\x64', 'S',    'T',    'O',    'C',    'K',    '1',
                                   ' ',    ' ',    '\x00', '\x00', '\x00', '\x05', '\x00', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x06'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::NonCrossTradeMessage>(messages[0]));
    auto msg = std::get<itch::NonCrossTradeMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.order_reference_number, 4);
    EXPECT_EQ(msg.buy_sell_indicator, 'B');
    EXPECT_EQ(msg.shares, 100);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.price, 5);
    EXPECT_EQ(msg.match_number, 6);
}

TEST(MessagesTest, CrossTradeMessage) {
    const char  raw_msg[] = {'\x00', '\x28', 'Q',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00',
                             '\x00', '\x00', '\x64', 'S',    'T',    'O',    'C',    'K',    '1',
                             ' ',    ' ',    '\x00', '\x00', '\x00', '\x05', '\x00', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x00', '\x06', 'C'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::CrossTradeMessage>(messages[0]));
    auto msg = std::get<itch::CrossTradeMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.shares, 100);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.cross_price, 5);
    EXPECT_EQ(msg.match_number, 6);
    EXPECT_EQ(msg.cross_type, 'C');
}

TEST(MessagesTest, BrokenTradeMessage) {
    const char        raw_msg[] = {'\x00', '\x13', 'B',    '\x00', '\x01', '\x00', '\x02',
                                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x04'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::BrokenTradeMessage>(messages[0]));
    auto msg = std::get<itch::BrokenTradeMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.match_number, 4);
}

TEST(MessagesTest, NOIIMessage) {
    const char  raw_msg[] = {'\x00', '\x32', 'I',    '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
                             '\x00', '\x00', '\x00', '\x03', '\x00', '\x00', '\x00', '\x00', '\x00',
                             '\x00', '\x00', '\x64', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                             '\x00', '\xc8', 'B',    'S',    'T',    'O',    'C',    'K',    '1',
                             ' ',    ' ',    '\x00', '\x00', '\x00', '\x05', '\x00', '\x00', '\x00',
                             '\x06', '\x00', '\x00', '\x00', '\x07', 'O',    ' '};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::NOIIMessage>(messages[0]));
    auto msg = std::get<itch::NOIIMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.paired_shares, 100);
    EXPECT_EQ(msg.imbalance_shares, 200);
    EXPECT_EQ(msg.imbalance_direction, 'B');
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.far_price, 5);
    EXPECT_EQ(msg.near_price, 6);
    EXPECT_EQ(msg.current_reference_price, 7);
    EXPECT_EQ(msg.cross_type, 'O');
    EXPECT_EQ(msg.price_variation_indicator, ' ');
}

TEST(MessagesTest, RetailPriceImprovementIndicatorMessage) {
    const char        raw_msg[] = {'\x00', '\x14', 'N',    '\x00', '\x01', '\x00', '\x02', '\x00',
                                   '\x00', '\x00', '\x00', '\x00', '\x03', 'S',    'T',    'O',
                                   'C',    'K',    '1',    ' ',    ' ',    'A'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto         messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::RetailPriceImprovementIndicatorMessage>(messages[0]));
    auto msg = std::get<itch::RetailPriceImprovementIndicatorMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(std::string(msg.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg.interest_flag, 'A');
}
