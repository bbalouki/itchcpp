#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "itch/book/l3_book.hpp"
#include "itch/messages.hpp"
#include "itch/tape.hpp"

namespace itch::book {

/// @brief Maintains a full-market set of order books from a single pass over the
///        feed.
///
/// Every order, execution, cancel, delete, and replace message carries a stock
/// locate code; the manager routes each message to that security's `L3Book` in
/// O(1) through a flat, locate-indexed table, reconstructing every symbol on the
/// feed at once rather than a single pre-selected one. It optionally restricts
/// work to a chosen universe of symbols, emits best-bid/offer change events as
/// they happen, and extracts the trade tape.
class BookManager {
   public:
    /// @brief Invoked when a book's best bid or offer changes.
    using BboCallback = std::function<void(const L3Book& book, const Bbo& bbo)>;

    BookManager() = default;

    /// @brief Processes one parsed ITCH message, updating the relevant book and
    ///        emitting BBO/trade events as appropriate.
    auto process(const Message& message) -> void;

    /// @brief Installs the best-bid/offer change callback (empty clears it).
    auto set_bbo_callback(BboCallback callback) -> void { m_bbo_callback = std::move(callback); }

    /// @brief Installs the trade-tape callback (empty clears it).
    auto set_trade_callback(TradeCallback callback) -> void {
        m_trade_callback = std::move(callback);
    }

    /// @brief Restricts tracking to the given symbol (call once per symbol). When
    ///        no symbol is added, every symbol on the feed is tracked.
    auto track_symbol(std::string symbol) -> void { m_universe.insert(std::move(symbol)); }

    /// @brief The book for a locate code, or nullptr if none is tracked.
    [[nodiscard]] auto book(std::uint16_t stock_locate) const -> const L3Book*;

    /// @brief The book for a symbol, or nullptr if none is tracked.
    [[nodiscard]] auto book_for_symbol(std::string_view symbol) const -> const L3Book*;

    /// @brief The number of books currently maintained.
    [[nodiscard]] auto book_count() const noexcept -> std::size_t { return m_book_count; }

    /// @brief The symbol associated with a locate code (empty if unknown).
    [[nodiscard]] auto symbol_for_locate(std::uint16_t stock_locate) const -> std::string_view;

   private:
    struct BookEntry {
        L3Book book;
        Bbo    last_bbo {};
    };

    // Returns the entry for a locate, creating it if the symbol is in-universe.
    auto ensure_entry(std::uint16_t stock_locate, std::string_view symbol) -> BookEntry*;
    // Returns the existing entry for a locate, or nullptr.
    [[nodiscard]] auto entry(std::uint16_t stock_locate) const -> BookEntry*;
    // Emits a BBO event if the book's top has changed since last seen.
    auto emit_bbo_if_changed(BookEntry& target) -> void;
    // Whether the symbol should be tracked given the configured universe.
    [[nodiscard]] auto in_universe(std::string_view symbol) const -> bool;

    std::vector<std::unique_ptr<BookEntry>> m_books_by_locate;   ///< Indexed by locate.
    std::vector<std::string>                m_symbol_by_locate;  ///< Locate -> symbol.
    std::unordered_set<std::string>         m_universe;          ///< Empty == track all.
    BboCallback                             m_bbo_callback {};
    TradeCallback                           m_trade_callback {};
    std::size_t                             m_book_count {0};
};

}  // namespace itch::book
