#include "itch/io/arrow_export.hpp"

#ifdef ITCH_WITH_ARROW

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

#include <string>
#include <variant>

#include "itch/price.hpp"

namespace itch::io {

struct ArrowExporter::Impl {
    arrow::StringBuilder message_type;
    arrow::UInt64Builder timestamp;
    arrow::UInt16Builder stock_locate;
    arrow::UInt16Builder tracking_number;
    arrow::StringBuilder symbol;
    arrow::UInt64Builder reference_number;
    arrow::StringBuilder side;
    arrow::UInt64Builder shares;
    arrow::DoubleBuilder price;
    arrow::UInt64Builder match_number;
    arrow::StringBuilder printable;
    arrow::StringBuilder extra;
    std::uint64_t        row_count {0};
    std::string          last_error;

    // Builds the common header columns shared by every message type.
    template <typename MsgType>
    auto append_common(const MsgType& msg) -> void {
        (void)message_type.Append(std::string {msg.message_type});
        (void)timestamp.Append(msg.timestamp);
        (void)stock_locate.Append(msg.stock_locate);
        (void)tracking_number.Append(msg.tracking_number);
    }

    // Appends nulls/empties for the type-specific columns, overwritten as needed.
    auto append_blank_specific() -> void {
        (void)symbol.AppendEmptyValue();
        (void)reference_number.AppendNull();
        (void)side.AppendEmptyValue();
        (void)shares.AppendNull();
        (void)price.AppendNull();
        (void)match_number.AppendNull();
        (void)printable.AppendEmptyValue();
        (void)extra.AppendEmptyValue();
    }
};

ArrowExporter::ArrowExporter() : m_impl {std::make_unique<Impl>()} {}
ArrowExporter::~ArrowExporter() = default;

auto ArrowExporter::rows() const noexcept -> std::uint64_t { return m_impl->row_count; }
auto ArrowExporter::error() const -> const std::string& { return m_impl->last_error; }

auto ArrowExporter::append(const Message& message) -> void {
    Impl& impl = *m_impl;
    std::visit([&impl](const auto& msg) { impl.append_common(msg); }, message);

    // Default every type-specific column to null/empty, then set what applies.
    // Replacing the convenience of AppendEmptyValue with explicit per-type code
    // keeps the column lengths in lockstep with the header columns.
    std::string   symbol_value;
    bool          has_reference = false;
    std::uint64_t reference     = 0;
    std::string   side_value;
    bool          has_shares   = false;
    std::uint64_t shares_value = 0;
    bool          has_price    = false;
    double        price_value  = 0.0;
    bool          has_match    = false;
    std::uint64_t match_value  = 0;
    std::string   printable_value;
    std::string   extra_value;

    if (const auto* add = std::get_if<AddOrderMessage>(&message)) {
        symbol_value  = to_string(add->stock, STOCK_LEN);
        has_reference = true;
        reference     = add->order_reference_number;
        side_value    = std::string {add->buy_sell_indicator};
        has_shares    = true;
        shares_value  = add->shares;
        has_price     = true;
        price_value   = StandardPrice {add->price}.to_double();
    } else if (const auto* add_mpid = std::get_if<AddOrderMPIDAttributionMessage>(&message)) {
        symbol_value  = to_string(add_mpid->stock, STOCK_LEN);
        has_reference = true;
        reference     = add_mpid->order_reference_number;
        side_value    = std::string {add_mpid->buy_sell_indicator};
        has_shares    = true;
        shares_value  = add_mpid->shares;
        has_price     = true;
        price_value   = StandardPrice {add_mpid->price}.to_double();
        extra_value   = "mpid=" + to_string(add_mpid->attribution, 4);
    } else if (const auto* exec = std::get_if<OrderExecutedMessage>(&message)) {
        has_reference = true;
        reference     = exec->order_reference_number;
        has_shares    = true;
        shares_value  = exec->executed_shares;
        has_match     = true;
        match_value   = exec->match_number;
    } else if (const auto* exec_px = std::get_if<OrderExecutedWithPriceMessage>(&message)) {
        has_reference   = true;
        reference       = exec_px->order_reference_number;
        has_shares      = true;
        shares_value    = exec_px->executed_shares;
        has_price       = true;
        price_value     = StandardPrice {exec_px->execution_price}.to_double();
        has_match       = true;
        match_value     = exec_px->match_number;
        printable_value = std::string {exec_px->printable};
    } else if (const auto* cancel = std::get_if<OrderCancelMessage>(&message)) {
        has_reference = true;
        reference     = cancel->order_reference_number;
        has_shares    = true;
        shares_value  = cancel->cancelled_shares;
    } else if (const auto* del = std::get_if<OrderDeleteMessage>(&message)) {
        has_reference = true;
        reference     = del->order_reference_number;
    } else if (const auto* replace = std::get_if<OrderReplaceMessage>(&message)) {
        has_reference = true;
        reference     = replace->new_order_reference_number;
        has_shares    = true;
        shares_value  = replace->shares;
        has_price     = true;
        price_value   = StandardPrice {replace->price}.to_double();
        extra_value   = "orig=" + std::to_string(replace->original_order_reference_number);
    } else if (const auto* trade = std::get_if<NonCrossTradeMessage>(&message)) {
        symbol_value  = to_string(trade->stock, STOCK_LEN);
        has_reference = true;
        reference     = trade->order_reference_number;
        side_value    = std::string {trade->buy_sell_indicator};
        has_shares    = true;
        shares_value  = trade->shares;
        has_price     = true;
        price_value   = StandardPrice {trade->price}.to_double();
        has_match     = true;
        match_value   = trade->match_number;
    } else if (const auto* cross = std::get_if<CrossTradeMessage>(&message)) {
        symbol_value = to_string(cross->stock, STOCK_LEN);
        has_shares   = true;
        shares_value = cross->shares;
        has_price    = true;
        price_value  = StandardPrice {cross->cross_price}.to_double();
        has_match    = true;
        match_value  = cross->match_number;
        extra_value  = std::string {"cross="} + cross->cross_type;
    } else if (const auto* event = std::get_if<SystemEventMessage>(&message)) {
        extra_value = std::string {"event="} + event->event_code;
    } else if (const auto* directory = std::get_if<StockDirectoryMessage>(&message)) {
        symbol_value = to_string(directory->stock, STOCK_LEN);
    }

    (void)impl.symbol.Append(symbol_value);
    if (has_reference) {
        (void)impl.reference_number.Append(reference);
    } else {
        (void)impl.reference_number.AppendNull();
    }
    (void)impl.side.Append(side_value);
    if (has_shares) {
        (void)impl.shares.Append(shares_value);
    } else {
        (void)impl.shares.AppendNull();
    }
    if (has_price) {
        (void)impl.price.Append(price_value);
    } else {
        (void)impl.price.AppendNull();
    }
    if (has_match) {
        (void)impl.match_number.Append(match_value);
    } else {
        (void)impl.match_number.AppendNull();
    }
    (void)impl.printable.Append(printable_value);
    (void)impl.extra.Append(extra_value);
    ++impl.row_count;
}

auto ArrowExporter::write_parquet(const std::string& path) -> bool {
    Impl& impl = *m_impl;

    auto finish =
        [&impl](arrow::ArrayBuilder& builder, std::shared_ptr<arrow::Array>& out) -> bool {
        const arrow::Status status = builder.Finish(&out);
        if (!status.ok()) {
            impl.last_error = status.ToString();
            return false;
        }
        return true;
    };

    std::shared_ptr<arrow::Array> columns[12];
    arrow::ArrayBuilder*          builders[12] = {
        &impl.message_type,
        &impl.timestamp,
        &impl.stock_locate,
        &impl.tracking_number,
        &impl.symbol,
        &impl.reference_number,
        &impl.side,
        &impl.shares,
        &impl.price,
        &impl.match_number,
        &impl.printable,
        &impl.extra
    };
    for (int index = 0; index < 12; ++index) {
        if (!finish(*builders[index], columns[index])) {
            return false;
        }
    }

    auto schema = arrow::schema({
        arrow::field("message_type", arrow::utf8()),
        arrow::field("timestamp", arrow::uint64()),
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("symbol", arrow::utf8()),
        arrow::field("reference_number", arrow::uint64()),
        arrow::field("side", arrow::utf8()),
        arrow::field("shares", arrow::uint64()),
        arrow::field("price", arrow::float64()),
        arrow::field("match_number", arrow::uint64()),
        arrow::field("printable", arrow::utf8()),
        arrow::field("extra", arrow::utf8()),
    });

    std::vector<std::shared_ptr<arrow::Array>> column_vector {columns, columns + 12};
    const auto                                 table = arrow::Table::Make(schema, column_vector);

    auto outfile_result = arrow::io::FileOutputStream::Open(path);
    if (!outfile_result.ok()) {
        impl.last_error = outfile_result.status().ToString();
        return false;
    }
    const auto status = parquet::arrow::WriteTable(
        *table,
        arrow::default_memory_pool(),
        *outfile_result,
        static_cast<std::int64_t>(impl.row_count > 0 ? impl.row_count : 1)
    );
    if (!status.ok()) {
        impl.last_error = status.ToString();
        return false;
    }
    return true;
}

}  // namespace itch::io

#endif  // ITCH_WITH_ARROW
