#pragma once

/// @file
/// @brief Full-market order book manager that fans out ITCH messages to
///        per-symbol `L3Book` instances.
///
/// This header declares `BookManager`, which owns a locate-indexed table of
/// `L3Book`s, routes each parsed ITCH message to the right book, optionally
/// restricts work to a configured symbol universe, and emits best-bid/offer
/// and trade events as they occur.
///
/// @author Bertin Balouki SIMYELI

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

    /// @brief Constructs an empty manager with no books tracked yet.
    BookManager() = default;

    /// @brief Processes one parsed ITCH message, updating the relevant book and
    ///        emitting BBO/trade events as appropriate.
    /// @param message The parsed ITCH message to apply.
    auto process(const Message& message) -> void;

    /// @brief Installs the best-bid/offer change callback (empty clears it).
    /// @param callback Invoked whenever a tracked book's best bid or offer
    ///        changes.
    auto set_bbo_callback(BboCallback callback) -> void { m_bbo_callback = std::move(callback); }

    /// @brief Installs the trade-tape callback (empty clears it).
    /// @param callback Invoked for each trade extracted from the feed.
    auto set_trade_callback(TradeCallback callback) -> void {
        m_trade_callback = std::move(callback);
    }

    /// @brief Restricts tracking to the given symbol (call once per symbol). When
    ///        no symbol is added, every symbol on the feed is tracked.
    /// @param symbol The ticker symbol to add to the tracked universe.
    auto track_symbol(std::string symbol) -> void { m_universe.insert(std::move(symbol)); }

    /// @brief The book for a locate code, or nullptr if none is tracked.
    /// @param stock_locate The exchange-assigned stock locate code.
    /// @return Pointer to the book for `stock_locate`, or nullptr if it is
    ///         not tracked.
    [[nodiscard]] auto book(std::uint16_t stock_locate) const -> const L3Book*;

    /// @brief The book for a symbol, or nullptr if none is tracked.
    /// @param symbol The ticker symbol to look up.
    /// @return Pointer to the book for `symbol`, or nullptr if it is not
    ///         tracked.
    [[nodiscard]] auto book_for_symbol(std::string_view symbol) const -> const L3Book*;

    /// @brief The number of books currently maintained.
    /// @return The count of books currently tracked.
    [[nodiscard]] auto book_count() const noexcept -> std::size_t { return m_book_count; }

    /// @brief The symbol associated with a locate code (empty if unknown).
    /// @param stock_locate The exchange-assigned stock locate code.
    /// @return The ticker symbol for `stock_locate`, or an empty view if
    ///         unknown.
    [[nodiscard]] auto symbol_for_locate(std::uint16_t stock_locate) const -> std::string_view;

   private:
    struct BookEntry {
        L3Book book;
        Bbo    last_bbo {};
    };

    /// @brief Returns the entry for a locate, creating it if the symbol is
    ///        in-universe.
    /// @param stock_locate The exchange-assigned stock locate code.
    /// @param symbol The ticker symbol associated with `stock_locate`.
    /// @return Pointer to the (possibly newly created) entry, or nullptr if
    ///         `symbol` is not in the tracked universe.
    auto ensure_entry(std::uint16_t stock_locate, std::string_view symbol) -> BookEntry*;

    /// @brief Returns the existing entry for a locate, or nullptr.
    /// @param stock_locate The exchange-assigned stock locate code.
    /// @return Pointer to the entry for `stock_locate`, or nullptr if none
    ///         exists.
    [[nodiscard]] auto entry(std::uint16_t stock_locate) const -> BookEntry*;

    /// @brief Emits a BBO event if the book's top has changed since last seen.
    /// @param target The book entry to check and, if changed, report.
    auto emit_bbo_if_changed(BookEntry& target) -> void;

    /// @brief Whether the symbol should be tracked given the configured
    ///        universe.
    /// @param symbol The ticker symbol to check.
    /// @return True if `symbol` should be tracked, false otherwise.
    [[nodiscard]] auto in_universe(std::string_view symbol) const -> bool;

    /// @brief Adds a new order to the appropriate book from an Add Order (or
    ///        MPID-attributed Add Order) message.
    /// @tparam AddMessage AddOrderMessage or AddOrderMPIDAttributionMessage.
    /// @param add The parsed add-order message.
    template <typename AddMessage>
    auto handle_add_order(const AddMessage& add) -> void;

    /// @brief Executes shares against a resting order and emits a trade using
    ///        the order's own price (Order Executed carries no price itself).
    /// @param exec The parsed order-executed message.
    auto handle_order_executed(const OrderExecutedMessage& exec) -> void;

    /// @brief Executes shares against a resting order at an explicit price
    ///        and emits a trade (Order Executed With Price).
    /// @param exec The parsed order-executed-with-price message.
    auto handle_order_executed_with_price(const OrderExecutedWithPriceMessage& exec) -> void;

    /// @brief Reduces a resting order's size (Order Cancel).
    /// @param cancel The parsed order-cancel message.
    auto handle_order_cancel(const OrderCancelMessage& cancel) -> void;

    /// @brief Removes a resting order from its book (Order Delete).
    /// @param del The parsed order-delete message.
    auto handle_order_delete(const OrderDeleteMessage& del) -> void;

    /// @brief Replaces a resting order with a new reference number, size, and
    ///        price (Order Replace).
    /// @param replace The parsed order-replace message.
    auto handle_order_replace(const OrderReplaceMessage& replace) -> void;

    /// @brief Extracts a trade-tape event from a non-displayed trade print
    ///        (Non-Cross Trade); does not alter the visible book.
    /// @param trade The parsed non-cross-trade message.
    auto handle_non_cross_trade(const NonCrossTradeMessage& trade) -> void;

    /// @brief Extracts a trade-tape event from a cross trade (Cross Trade).
    /// @param cross The parsed cross-trade message.
    auto handle_cross_trade(const CrossTradeMessage& cross) -> void;

    /// @brief Records the symbol associated with a locate code (Stock
    ///        Directory).
    /// @param directory The parsed stock-directory message.
    auto handle_stock_directory(const StockDirectoryMessage& directory) -> void;

    std::vector<std::unique_ptr<BookEntry>> m_books_by_locate;   ///< Indexed by locate.
    std::vector<std::string>                m_symbol_by_locate;  ///< Locate -> symbol.
    std::unordered_set<std::string>         m_universe;          ///< Empty == track all.
    BboCallback                             m_bbo_callback {};
    TradeCallback                           m_trade_callback {};
    std::size_t                             m_book_count {0};
};

}  // namespace itch::book
