#pragma once

/// @file
/// @brief CSV output sink for parsed ITCH messages.
///
/// `CsvSink` is the dependency-free, universal output format: it flattens the
/// heterogeneous ITCH message set into one wide, normalized row schema suitable
/// for quick inspection and legacy tooling.
///
/// @author Bertin Balouki SIMYELI

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
    ///
    /// @param out Output stream the CSV rows are written to.
    /// @param write_header Whether to emit the CSV header row on construction.
    explicit CsvSink(std::ostream& out, bool write_header = true);

    /// @brief Writes one message as a CSV row.
    ///
    /// @param message The parsed message to write.
    auto write(const Message& message) -> void override;

    /// @brief Flushes the underlying stream.
    auto flush() -> void override;

    /// @brief The number of rows written so far.
    ///
    /// @return The count of rows written since construction.
    [[nodiscard]] auto rows_written() const noexcept -> std::uint64_t { return m_rows_written; }

   private:
    std::ostream& m_out;
    std::uint64_t m_rows_written {0};
};

}  // namespace itch::io
