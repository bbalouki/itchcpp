#include "itch/io/csv_sink.hpp"

#include <format>
#include <string>
#include <variant>

#include "itch/price.hpp"

namespace itch::io {

namespace {

/// @brief The flattened CSV view of one message, with blanks for absent fields.
struct Row {
    char          message_type {'\0'};
    std::uint64_t timestamp {0};
    std::uint16_t stock_locate {0};
    std::uint16_t tracking_number {0};
    std::string   symbol;
    std::string   reference_number;
    char          side {'\0'};
    std::string   shares;
    std::string   price;
    std::string   match_number;
    char          printable {'\0'};
    std::string   extra;
};

// Builds the common header fields shared by every message.
template <typename MsgType>
auto fill_common(Row& row, const MsgType& msg) -> void {
    row.message_type    = msg.message_type;
    row.timestamp       = msg.timestamp;
    row.stock_locate    = msg.stock_locate;
    row.tracking_number = msg.tracking_number;
}

// Quotes a value only if it contains a comma; symbols never do, but be safe.
auto field(const std::string& value) -> std::string {
    // std::string::contains needs C++23, but this file must also build under
    // this project's C++20 floor.
    // NOLINTNEXTLINE(readability-container-contains)
    if (value.find(',') != std::string::npos) {
        return std::format("\"{}\"", value);
    }
    return value;
}

auto build_row(const Message& message) -> Row {
    Row row {};
    std::visit([&row](const auto& msg) { fill_common(row, msg); }, message);

    if (const auto* add = std::get_if<AddOrderMessage>(&message)) {
        row.symbol           = to_string(add->stock, STOCK_LEN);
        row.reference_number = std::to_string(add->order_reference_number);
        row.side             = add->buy_sell_indicator;
        row.shares           = std::to_string(add->shares);
        row.price            = StandardPrice {add->price}.to_string();
    } else if (const auto* add_mpid = std::get_if<AddOrderMPIDAttributionMessage>(&message)) {
        row.symbol           = to_string(add_mpid->stock, STOCK_LEN);
        row.reference_number = std::to_string(add_mpid->order_reference_number);
        row.side             = add_mpid->buy_sell_indicator;
        row.shares           = std::to_string(add_mpid->shares);
        row.price            = StandardPrice {add_mpid->price}.to_string();
        row.extra            = std::format("mpid={}", to_string(add_mpid->attribution, 4));
    } else if (const auto* exec = std::get_if<OrderExecutedMessage>(&message)) {
        row.reference_number = std::to_string(exec->order_reference_number);
        row.shares           = std::to_string(exec->executed_shares);
        row.match_number     = std::to_string(exec->match_number);
    } else if (const auto* exec_px = std::get_if<OrderExecutedWithPriceMessage>(&message)) {
        row.reference_number = std::to_string(exec_px->order_reference_number);
        row.shares           = std::to_string(exec_px->executed_shares);
        row.price            = StandardPrice {exec_px->execution_price}.to_string();
        row.match_number     = std::to_string(exec_px->match_number);
        row.printable        = exec_px->printable;
    } else if (const auto* cancel = std::get_if<OrderCancelMessage>(&message)) {
        row.reference_number = std::to_string(cancel->order_reference_number);
        row.shares           = std::to_string(cancel->cancelled_shares);
    } else if (const auto* del = std::get_if<OrderDeleteMessage>(&message)) {
        row.reference_number = std::to_string(del->order_reference_number);
    } else if (const auto* replace = std::get_if<OrderReplaceMessage>(&message)) {
        row.reference_number = std::to_string(replace->new_order_reference_number);
        row.shares           = std::to_string(replace->shares);
        row.price            = StandardPrice {replace->price}.to_string();
        row.extra            = std::format("orig={}", replace->original_order_reference_number);
    } else if (const auto* trade = std::get_if<NonCrossTradeMessage>(&message)) {
        row.symbol           = to_string(trade->stock, STOCK_LEN);
        row.reference_number = std::to_string(trade->order_reference_number);
        row.side             = trade->buy_sell_indicator;
        row.shares           = std::to_string(trade->shares);
        row.price            = StandardPrice {trade->price}.to_string();
        row.match_number     = std::to_string(trade->match_number);
    } else if (const auto* cross = std::get_if<CrossTradeMessage>(&message)) {
        row.symbol       = to_string(cross->stock, STOCK_LEN);
        row.shares       = std::to_string(cross->shares);
        row.price        = StandardPrice {cross->cross_price}.to_string();
        row.match_number = std::to_string(cross->match_number);
        row.extra        = std::format("cross={}", cross->cross_type);
    } else if (const auto* event = std::get_if<SystemEventMessage>(&message)) {
        row.extra = std::format("event={}", event->event_code);
    } else if (const auto* directory = std::get_if<StockDirectoryMessage>(&message)) {
        row.symbol = to_string(directory->stock, STOCK_LEN);
        row.extra  = std::format("category={}", directory->market_category);
    }
    return row;
}

// Renders a single character field, blank when unset.
auto char_field(char value) -> std::string {
    return value == '\0' ? std::string {} : std::string {value};
}

}  // namespace

CsvSink::CsvSink(std::ostream& out, bool write_header) : m_out {out} {
    if (write_header) {
        m_out << "message_type,timestamp,stock_locate,tracking_number,symbol,reference_number,"
                 "side,shares,price,match_number,printable,extra\n";
    }
}

auto CsvSink::write(const Message& message) -> void {
    const Row row = build_row(message);
    m_out << std::format(
        "{},{},{},{},{},{},{},{},{},{},{},{}\n",
        row.message_type,
        row.timestamp,
        row.stock_locate,
        row.tracking_number,
        field(row.symbol),
        row.reference_number,
        char_field(row.side),
        row.shares,
        row.price,
        row.match_number,
        char_field(row.printable),
        field(row.extra)
    );
    ++m_rows_written;
}

auto CsvSink::flush() -> void { m_out.flush(); }

}  // namespace itch::io
