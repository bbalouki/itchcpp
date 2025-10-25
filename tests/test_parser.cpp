#include <gtest/gtest.h>

#include <numeric>
#include <sstream>
#include <vector>

#include "parser.hpp"

template <typename T>
std::vector<char> to_big_endian(T val) {
    static_assert(std::is_integral_v<T>, "to_big_endian requires an integral type");
    auto        value = itch::utils::swap_bytes(val);
    const char* bytes = reinterpret_cast<const char*>(&value);
    return {bytes, bytes + sizeof(T)};
}

template <typename T>
std::string create_test_buffer(const T& msg) {
    uint16_t length = sizeof(T);

    std::vector<char> length_bytes = to_big_endian(length);
    std::vector<char> buffer;
    buffer.insert(buffer.end(), length_bytes.begin(), length_bytes.end());

    const char* msg_bytes = reinterpret_cast<const char*>(&msg);
    buffer.insert(buffer.end(), msg_bytes, msg_bytes + sizeof(T));

    return {buffer.begin(), buffer.end()};
}

std::string create_test_buffer(const itch::SystemEventMessage& msg) {
    std::vector<char> payload;
    payload.push_back('S');

    auto stock_locate = to_big_endian(msg.stock_locate);
    payload.insert(payload.end(), stock_locate.begin(), stock_locate.end());
    auto tracking_number = to_big_endian(msg.tracking_number);
    payload.insert(payload.end(), tracking_number.begin(), tracking_number.end());

    auto timestamp = to_big_endian(msg.timestamp);
    payload.insert(payload.end(), timestamp.begin() + 2, timestamp.end());
    payload.push_back(msg.event_code);

    uint16_t          length       = payload.size();
    std::vector<char> length_bytes = to_big_endian(length);

    std::string buffer(length_bytes.begin(), length_bytes.end());
    buffer.append(payload.begin(), payload.end());

    return buffer;
}

std::string create_test_buffer(const itch::AddOrderMessage& msg) {
    std::vector<char> payload;
    payload.push_back('A');

    auto stock_locate = to_big_endian(msg.stock_locate);
    payload.insert(payload.end(), stock_locate.begin(), stock_locate.end());

    auto tracking_number = to_big_endian(msg.tracking_number);
    payload.insert(payload.end(), tracking_number.begin(), tracking_number.end());

    auto timestamp = to_big_endian(msg.timestamp);
    payload.insert(payload.end(), timestamp.begin() + 2, timestamp.end());
    auto order_ref = to_big_endian(msg.order_reference_number);
    payload.insert(payload.end(), order_ref.begin(), order_ref.end());

    payload.push_back(msg.buy_sell_indicator);
    auto shares = to_big_endian(msg.shares);
    payload.insert(payload.end(), shares.begin(), shares.end());

    payload.insert(payload.end(), msg.stock, msg.stock + 8);
    auto price = to_big_endian(msg.price);
    payload.insert(payload.end(), price.begin(), price.end());

    uint16_t          length       = payload.size();
    std::vector<char> length_bytes = to_big_endian(length);

    std::string buffer(length_bytes.begin(), length_bytes.end());
    buffer.append(payload.begin(), payload.end());

    return buffer;
}

class ParserTest : public ::testing::Test {
   protected:
    itch::Parser parser;
};

TEST_F(ParserTest, SingleValidSystemEventMessage) {
    itch::SystemEventMessage msg_to_pack{};
    msg_to_pack.stock_locate    = 1;
    msg_to_pack.tracking_number = 2;
    msg_to_pack.timestamp       = 3;
    msg_to_pack.event_code      = 'O';

    std::string       data = create_test_buffer(msg_to_pack);
    std::stringstream ss(data);

    auto messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));

    auto msg = std::get<itch::SystemEventMessage>(messages[0]);
    EXPECT_EQ(msg.stock_locate, 1);
    EXPECT_EQ(msg.tracking_number, 2);
    EXPECT_EQ(msg.timestamp, 3);
    EXPECT_EQ(msg.event_code, 'O');
}

TEST_F(ParserTest, MultipleValidMessages) {
    itch::SystemEventMessage msg1_to_pack{};
    msg1_to_pack.stock_locate = 1;
    msg1_to_pack.timestamp    = 3;
    msg1_to_pack.event_code   = 'O';

    itch::AddOrderMessage msg2_to_pack{};
    msg2_to_pack.order_reference_number = 12345;
    msg2_to_pack.buy_sell_indicator     = 'B';
    msg2_to_pack.shares                 = 100;
    memcpy(msg2_to_pack.stock, "AAPL    ", 8);
    msg2_to_pack.price      = 1500000;
    std::string       data1 = create_test_buffer(msg1_to_pack);
    std::string       data2 = create_test_buffer(msg2_to_pack);
    std::stringstream ss(data1 + data2);

    auto messages = parser.parse(ss);

    ASSERT_EQ(messages.size(), 2);

    ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(messages[0]));
    auto msg1 = std::get<itch::SystemEventMessage>(messages[0]);
    EXPECT_EQ(msg1.stock_locate, 1);
    EXPECT_EQ(msg1.timestamp, 3);
    EXPECT_EQ(msg1.event_code, 'O');

    ASSERT_TRUE(std::holds_alternative<itch::AddOrderMessage>(messages[1]));
    auto msg2 = std::get<itch::AddOrderMessage>(messages[1]);
    EXPECT_EQ(msg2.order_reference_number, 12345);
    EXPECT_EQ(msg2.shares, 100);
    EXPECT_EQ(itch::to_string(msg2.stock, 8), "AAPL");
    EXPECT_EQ(msg2.price, 1500000);
}

TEST_F(ParserTest, ThrowsOnIncompletePayload) {
    // Create a valid message, then truncate it
    std::string data = create_test_buffer(itch::SystemEventMessage{});
    data.pop_back();  // Make the payload incomplete

    std::stringstream ss(data);

    EXPECT_THROW(parser.parse(ss), std::runtime_error);
}

TEST_F(ParserTest, ThrowsOnIncompleteHeader) {
    const char        raw_msg[] = {'\x00'};
    std::string       data(raw_msg, sizeof(raw_msg));
    std::stringstream ss(data);

    EXPECT_THROW(parser.parse(ss), std::runtime_error);
}

TEST_F(ParserTest, HandlesEmptyStream) {
    std::string       data;
    std::stringstream ss(data);
    auto              messages = parser.parse(ss);
    ASSERT_EQ(messages.size(), 0);
}

TEST_F(ParserTest, CallbackBasedParsing) {
    std::string data = create_test_buffer(itch::SystemEventMessage{}) +
                       create_test_buffer(itch::AddOrderMessage{});
    std::stringstream ss(data);

    size_t message_count = 0;
    auto   callback      = [&](const itch::Message& msg) {
        message_count++;
        if (message_count == 1) {
            ASSERT_TRUE(std::holds_alternative<itch::SystemEventMessage>(msg));
        } else if (message_count == 2) {
            ASSERT_TRUE(std::holds_alternative<itch::AddOrderMessage>(msg));
        }
    };

    parser.parse(ss, callback);
    EXPECT_EQ(message_count, 2);
}

TEST_F(ParserTest, FilteredParsing) {
    std::string data = create_test_buffer(itch::SystemEventMessage{}) +
                       create_test_buffer(itch::StockDirectoryMessage{}) +
                       create_test_buffer(itch::AddOrderMessage{});
    std::stringstream ss(data);
    auto              messages = parser.parse(ss, {'R'});

    ASSERT_EQ(messages.size(), 1);
    ASSERT_TRUE(std::holds_alternative<itch::StockDirectoryMessage>(messages[0]));
}

TEST_F(ParserTest, SkipsZeroLengthMessage) {
    const char zero_len_msg[] = {'\x00', '\x00'};

    std::string data = create_test_buffer(itch::SystemEventMessage{}) +
                       std::string(zero_len_msg, 2) +
                       create_test_buffer(itch::SystemEventMessage{});

    std::stringstream ss(data);

    auto messages = parser.parse(ss);
    ASSERT_EQ(messages.size(), 2);
}

TEST_F(ParserTest, IgnoresTrailingGarbageData) {
    std::string       data = create_test_buffer(itch::SystemEventMessage{}) + "garbage";
    std::stringstream ss(data);

    // It throws because the "garbage" is interpreted as an incomplete header
    EXPECT_THROW(parser.parse(ss), std::runtime_error);

    // To test ignoring trailing data, ensure it's smaller than a header
    std::string       data2 = create_test_buffer(itch::SystemEventMessage{}) + "g";
    std::stringstream ss2(data2);
    EXPECT_THROW(parser.parse(ss2), std::runtime_error);

    std::vector<itch::Message> messages;
    EXPECT_THROW(messages = parser.parse(ss), std::runtime_error);
}

TEST_F(ParserTest, HandlesStreamEndingExactlyOnBoundary) {
    std::string data = create_test_buffer(itch::SystemEventMessage{}) +
                       create_test_buffer(itch::AddOrderMessage{});
    std::stringstream ss(data);

    // Should parse cleanly with no exceptions
    std::vector<itch::Message> messages;
    EXPECT_NO_THROW(messages = parser.parse(ss));
    ASSERT_EQ(messages.size(), 2);
}
