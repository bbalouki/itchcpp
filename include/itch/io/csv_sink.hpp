#pragma once

#include <cstdint>
#include <ostream>

#include "itch/io/sink.hpp"
#include "itch/messages.hpp"

namespace itch::io {

/// @brief Writes parsed ITCH messages to a CSV stream as a flat, normalized table.
///
/// ITCH messages are heterogeneous, so the sink flattens them into one wide row
/// schema sharing the common header plus the most frequently used per-type fields.
/// Fields a given message does not carry are left blank. This is the universal,
/// dependency-free output for quick inspection and legacy tooling; for columnar
/// analytics use the Arrow/Parquet exporter instead.
///
/// Columns: `message_type,timestamp,stock_locate,tracking_number,symbol,`
/// `reference_number,side,shares,price,match_number,printable,extra`.
class CsvSink : public MessageSink {
   public:
    /// @brief Constructs a sink writing to `out`, emitting the header row unless
    ///        `write_header` is false (useful when appending to an existing file).
    explicit CsvSink(std::ostream& out, bool write_header = true);

    /// @brief Writes one message as a CSV row.
    auto write(const Message& message) -> void override;

    /// @brief Flushes the underlying stream.
    auto flush() -> void override;

    /// @brief The number of rows written so far.
    [[nodiscard]] auto rows_written() const noexcept -> std::uint64_t { return m_rows_written; }

   private:
    std::ostream& m_out;
    std::uint64_t m_rows_written {0};
};

}  // namespace itch::io
