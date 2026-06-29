#pragma once

/// @file
/// @brief Apache Arrow / Parquet columnar export.
///
/// This header is only meaningful when the library is built with
/// `-DITCH_WITH_ARROW=ON` (Arrow and Parquet provided via vcpkg). When the option
/// is off the class is not compiled, keeping the core dependency-free. The export
/// turns a feed into a columnar table that drops straight into pandas, Polars,
/// DuckDB, and Spark pipelines.

#ifdef ITCH_WITH_ARROW

#include <cstdint>
#include <memory>
#include <string>

#include "itch/messages.hpp"

namespace arrow {
class Array;
}  // namespace arrow

namespace itch::io {

/// @brief Accumulates parsed messages into Arrow columns and writes Parquet.
///
/// Messages are flattened into the same normalized wide schema as `CsvSink`:
/// `message_type, timestamp, stock_locate, tracking_number, symbol,`
/// `reference_number, side, shares, price, match_number, printable, extra`.
/// Append each message with `append`, then call `write_parquet` to flush a file.
class ArrowExporter {
   public:
    ArrowExporter();
    ~ArrowExporter();
    ArrowExporter(const ArrowExporter&)                        = delete;
    auto operator=(const ArrowExporter&) -> ArrowExporter&     = delete;
    ArrowExporter(ArrowExporter&&) noexcept                    = default;
    auto operator=(ArrowExporter&&) noexcept -> ArrowExporter& = default;

    /// @brief Appends one message as a row to the column builders.
    auto append(const Message& message) -> void;

    /// @brief The number of rows appended so far.
    [[nodiscard]] auto rows() const noexcept -> std::uint64_t;

    /// @brief Finishes the builders and writes a Parquet file to `path`.
    ///
    /// @return True on success; on failure `error()` holds a description.
    [[nodiscard]] auto write_parquet(const std::string& path) -> bool;

    /// @brief A description of the last failure, or empty on success.
    [[nodiscard]] auto error() const -> const std::string&;

   private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace itch::io

#endif  // ITCH_WITH_ARROW
