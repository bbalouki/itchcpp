#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "parser.hpp"

TEST(ParserTest, SingleValidSystemEventMessage) {
    const char raw_msg[] = {'\x00', '\x0c', 'S',    '\x00', '\x01',
                            '\x00', '\x02', '\x00', '\x00', '\x00',
                            '\x00', '\x00', '\x03', 'O'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    auto messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);

    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));
    auto msg = std::get<itch::SystemEventMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.event_code, 'O');
}

TEST(ParserTest, MultipleValidMessages) {
    const char raw_msgs[] = {
        // Message 1: SystemEventMessage
        '\x00', '\x0c', 'S', '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x03', 'O',
        // Message 2: StockDirectoryMessage
        '\x00', '\x27', 'R', '\x00', '\x04', '\x00', '\x05', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x06', 'S', 'T', 'O', 'C', 'K', '1', ' ', ' ',
        'C', 'N', '\x00', '\x00', '\x00', '\x64', 'N', 'C', ' ', ' ', 'P', 'N',
        'N', ' ', 'N', '\x00', '\x00', '\x00', '\x00', 'N'};

    std::string data(raw_msgs, sizeof(raw_msgs));
    std::stringstream ss(data);

    itch::Parser parser;
    auto messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 2);

    // Verify first message
    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));
    auto msg1 = std::get<itch::SystemEventMessage>(messages[0]);
    EXPECT_EQ(msg1.stock_locate, 1);
    EXPECT_EQ(msg1.tracking_number, 2);
    EXPECT_EQ(msg1.timestamp, 3);
    EXPECT_EQ(msg1.event_code, 'O');

    // Verify second message
    ASSERT_TRUE(
        std::holds_alternative<itch::StockDirectoryMessage>(messages[1]));
    auto msg2 = std::get<itch::StockDirectoryMessage>(messages[1]);
    EXPECT_EQ(msg2.stock_locate, 4);
    EXPECT_EQ(msg2.tracking_number, 5);
    EXPECT_EQ(msg2.timestamp, 6);
    EXPECT_EQ(std::string(msg2.stock, 8), "STOCK1  ");
    EXPECT_EQ(msg2.market_category, 'C');
    EXPECT_EQ(msg2.round_lot_size, 100);
}

TEST(ParserTest, UnknownMessageType) {
    const char raw_msgs[] = {
        // Message 1: SystemEventMessage
        '\x00',
        '\x0c',
        'S',
        '\x00',
        '\x01',
        '\x00',
        '\x02',
        '\x00',
        '\x00',
        '\x00',
        '\x00',
        '\x00',
        '\x03',
        'O',
        // Unknown Message
        '\x00',
        '\x05',
        'Z',
        'd',
        'a',
        't',
        'a',
        // Message 2: SystemEventMessage
        '\x00',
        '\x0c',
        'S',
        '\x00',
        '\x04',
        '\x00',
        '\x05',
        '\x00',
        '\x00',
        '\x00',
        '\x00',
        '\x00',
        '\x06',
        'C',
    };

    std::string data(raw_msgs, sizeof(raw_msgs));
    std::stringstream ss(data);

    itch::Parser parser;
    auto messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 2);
    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));
    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[1]));
}

TEST(ParserTest, IncompleteMessage) {
    const char raw_msg[] = {
        '\x00', '\x0c', 'S', '\x00', '\x01', '\x00', '\x02',
    };
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    EXPECT_THROW(parser.parse(ss), std::runtime_error);
}

TEST(ParserTest, PartialMessage) {
    const char raw_msg[] = {'\x00', '\x0c', 'S',    '\x00', '\x01',
                            '\x00', '\x02', '\x00', '\x00', '\x00',
                            '\x00', '\x00', '\x03'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    EXPECT_THROW(parser.parse(ss), std::runtime_error);
}

TEST(ParserTest, EmptyStream) {
    std::string data;
    std::stringstream ss(data);

    itch::Parser parser;
    auto messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 0);
}

TEST(ParserTest, CallbackBasedParsing) {
    const char raw_msgs[] = {
        // Message 1: SystemEventMessage
        '\x00', '\x0c', 'S', '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x03', 'O',
        // Message 2: StockDirectoryMessage
        '\x00', '\x27', 'R', '\x00', '\x04', '\x00', '\x05', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x06', 'S', 'T', 'O', 'C', 'K', '1', ' ', ' ',
        'C', 'N', '\x00', '\x00', '\x00', '\x64', 'N', 'C', ' ', ' ', 'P', 'N',
        'N', ' ', 'N', '\x00', '\x00', '\x00', '\x00', 'N'};

    std::string data(raw_msgs, sizeof(raw_msgs));
    std::stringstream ss(data);

    itch::Parser parser;
    size_t message_count = 0;
    parser.parse(ss, [&](const itch::Message& msg) {
        message_count++;
        if (message_count == 1) {
            ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(msg));
        } else if (message_count == 2) {
            ASSERT_TRUE(
                std::holds_alternative<itch::StockDirectoryMessage>(msg));
        }
    });

    EXPECT_EQ(message_count, 2);
}

TEST(ParserTest, FilteredParsing) {
    const char raw_msgs[] = {
        // Message 1: SystemEventMessage
        '\x00', '\x0c', 'S', '\x00', '\x01', '\x00', '\x02', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x03', 'O',
        // Message 2: StockDirectoryMessage
        '\x00', '\x27', 'R', '\x00', '\x04', '\x00', '\x05', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x06', 'S', 'T', 'O', 'C', 'K', '1', ' ', ' ',
        'C', 'N', '\x00', '\x00', '\x00', '\x64', 'N', 'C', ' ', ' ', 'P', 'N',
        'N', ' ', 'N', '\x00', '\x00', '\x00', '\x00', 'N'};

    std::string data(raw_msgs, sizeof(raw_msgs));
    std::stringstream ss(data);

    itch::Parser parser;
    auto messages = parser.parse(ss, {'S'});

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));
}

TEST(ParserTest, CorruptedMessage) {
    const char raw_msg[] = {'\x00', '\x0a', 'S',    '\x00', '\x01',
                            '\x00', '\x02', '\x00', '\x00', '\x00',
                            '\x00', '\x00', '\x03', 'O'};
    std::string data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    itch::Parser parser;
    EXPECT_THROW(parser.parse(ss), std::runtime_error);
}
