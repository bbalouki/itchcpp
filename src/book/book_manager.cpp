#include "itch/book/book_manager.hpp"

#include <utility>
#include <variant>

namespace itch::book {

namespace {

// Maps an ITCH buy/sell indicator byte to the book Side enum.
auto to_side(char buy_sell_indicator) noexcept -> Side {
    return buy_sell_indicator == 'B' ? Side::buy : Side::sell;
}

}  // namespace

auto BookManager::in_universe(std::string_view symbol) const -> bool {
    return m_universe.empty() || m_universe.contains(std::string {symbol});
}

auto BookManager::entry(std::uint16_t stock_locate) const -> BookEntry* {
    if (stock_locate >= m_books_by_locate.size()) {
        return nullptr;
    }
    return m_books_by_locate[stock_locate].get();
}

auto BookManager::ensure_entry(std::uint16_t stock_locate, std::string_view symbol) -> BookEntry* {
    if (stock_locate >= m_books_by_locate.size()) {
        m_books_by_locate.resize(static_cast<std::size_t>(stock_locate) + 1);
    }
    if (auto& slot = m_books_by_locate[stock_locate]) {
        if (slot->book.symbol().empty() && !symbol.empty()) {
            slot->book.set_symbol(std::string {symbol});
        }
        return slot.get();
    }
    if (!in_universe(symbol)) {
        return nullptr;
    }
    m_books_by_locate[stock_locate] = std::make_unique<BookEntry>();
    m_books_by_locate[stock_locate]->book.set_symbol(std::string {symbol});
    ++m_book_count;
    return m_books_by_locate[stock_locate].get();
}

auto BookManager::emit_bbo_if_changed(BookEntry& target) -> void {
    const Bbo current = target.book.bbo();
    if (current != target.last_bbo) {
        target.last_bbo = current;
        if (m_bbo_callback) {
            m_bbo_callback(target.book, current);
        }
    }
}

template <typename AddMessage>
auto BookManager::handle_add_order(const AddMessage& add) -> void {
    BookEntry* target = ensure_entry(add.stock_locate, to_string(add.stock, STOCK_LEN));
    if (target != nullptr) {
        target->book.add_order(
            add.order_reference_number, to_side(add.buy_sell_indicator), add.shares, add.price
        );
        emit_bbo_if_changed(*target);
    }
}

auto BookManager::handle_order_executed(const OrderExecutedMessage& exec) -> void {
    BookEntry* target = entry(exec.stock_locate);
    if (target == nullptr) {
        return;
    }
    if (m_trade_callback) {
        const auto price = target->book.order_price(exec.order_reference_number);
        const auto side  = target->book.order_side(exec.order_reference_number);
        if (price.has_value()) {
            Trade trade {};
            trade.timestamp    = exec.timestamp;
            trade.stock_locate = exec.stock_locate;
            trade.symbol       = target->book.symbol();
            trade.price        = StandardPrice {*price};
            trade.shares       = exec.executed_shares;
            trade.match_number = exec.match_number;
            trade.side         = side.has_value() ? static_cast<char>(*side) : '\0';
            trade.printable    = true;
            m_trade_callback(trade);
        }
    }
    target->book.execute_order(exec.order_reference_number, exec.executed_shares);
    emit_bbo_if_changed(*target);
}

auto BookManager::handle_order_executed_with_price(const OrderExecutedWithPriceMessage& exec
) -> void {
    BookEntry* target = entry(exec.stock_locate);
    if (target == nullptr) {
        return;
    }
    if (m_trade_callback) {
        const auto side = target->book.order_side(exec.order_reference_number);
        Trade      trade {};
        trade.timestamp    = exec.timestamp;
        trade.stock_locate = exec.stock_locate;
        trade.symbol       = target->book.symbol();
        trade.price        = StandardPrice {exec.execution_price};
        trade.shares       = exec.executed_shares;
        trade.match_number = exec.match_number;
        trade.side         = side.has_value() ? static_cast<char>(*side) : '\0';
        trade.printable    = exec.printable == 'Y';
        m_trade_callback(trade);
    }
    target->book.execute_order(exec.order_reference_number, exec.executed_shares);
    emit_bbo_if_changed(*target);
}

auto BookManager::handle_order_cancel(const OrderCancelMessage& cancel) -> void {
    if (BookEntry* target = entry(cancel.stock_locate)) {
        target->book.reduce_order(cancel.order_reference_number, cancel.cancelled_shares);
        emit_bbo_if_changed(*target);
    }
}

auto BookManager::handle_order_delete(const OrderDeleteMessage& del) -> void {
    if (BookEntry* target = entry(del.stock_locate)) {
        target->book.delete_order(del.order_reference_number);
        emit_bbo_if_changed(*target);
    }
}

auto BookManager::handle_order_replace(const OrderReplaceMessage& replace) -> void {
    if (BookEntry* target = entry(replace.stock_locate)) {
        target->book.replace_order(
            replace.original_order_reference_number,
            replace.new_order_reference_number,
            replace.shares,
            replace.price
        );
        emit_bbo_if_changed(*target);
    }
}

auto BookManager::handle_non_cross_trade(const NonCrossTradeMessage& trade) -> void {
    // A non-displayable order print does not alter the visible book.
    if (!m_trade_callback) {
        return;
    }
    Trade tape_entry {};
    tape_entry.timestamp    = trade.timestamp;
    tape_entry.stock_locate = trade.stock_locate;
    tape_entry.symbol       = to_string(trade.stock, STOCK_LEN);
    tape_entry.price        = StandardPrice {trade.price};
    tape_entry.shares       = trade.shares;
    tape_entry.match_number = trade.match_number;
    tape_entry.side         = trade.buy_sell_indicator;
    tape_entry.printable    = true;
    m_trade_callback(tape_entry);
}

auto BookManager::handle_cross_trade(const CrossTradeMessage& cross) -> void {
    if (!m_trade_callback) {
        return;
    }
    Trade tape_entry {};
    tape_entry.timestamp    = cross.timestamp;
    tape_entry.stock_locate = cross.stock_locate;
    tape_entry.symbol       = to_string(cross.stock, STOCK_LEN);
    tape_entry.price        = StandardPrice {cross.cross_price};
    tape_entry.shares       = cross.shares;
    tape_entry.match_number = cross.match_number;
    tape_entry.printable    = true;
    tape_entry.is_cross     = true;
    tape_entry.cross_type   = cross.cross_type;
    m_trade_callback(tape_entry);
}

auto BookManager::handle_stock_directory(const StockDirectoryMessage& directory) -> void {
    if (directory.stock_locate >= m_symbol_by_locate.size()) {
        m_symbol_by_locate.resize(static_cast<std::size_t>(directory.stock_locate) + 1);
    }
    m_symbol_by_locate[directory.stock_locate] = to_string(directory.stock, STOCK_LEN);
}

auto BookManager::process(const Message& message) -> void {
    if (const auto* add = std::get_if<AddOrderMessage>(&message)) {
        handle_add_order(*add);
    } else if (const auto* add_mpid = std::get_if<AddOrderMPIDAttributionMessage>(&message)) {
        handle_add_order(*add_mpid);
    } else if (const auto* executed = std::get_if<OrderExecutedMessage>(&message)) {
        handle_order_executed(*executed);
    } else if (const auto* executed_with_price =
                   std::get_if<OrderExecutedWithPriceMessage>(&message)) {
        handle_order_executed_with_price(*executed_with_price);
    } else if (const auto* cancel = std::get_if<OrderCancelMessage>(&message)) {
        handle_order_cancel(*cancel);
    } else if (const auto* del = std::get_if<OrderDeleteMessage>(&message)) {
        handle_order_delete(*del);
    } else if (const auto* replace = std::get_if<OrderReplaceMessage>(&message)) {
        handle_order_replace(*replace);
    } else if (const auto* trade = std::get_if<NonCrossTradeMessage>(&message)) {
        handle_non_cross_trade(*trade);
    } else if (const auto* cross = std::get_if<CrossTradeMessage>(&message)) {
        handle_cross_trade(*cross);
    } else if (const auto* directory = std::get_if<StockDirectoryMessage>(&message)) {
        handle_stock_directory(*directory);
    }
}

template auto BookManager::handle_add_order(const AddOrderMessage&) -> void;
template auto BookManager::handle_add_order(const AddOrderMPIDAttributionMessage&) -> void;

auto BookManager::book(std::uint16_t stock_locate) const -> const L3Book* {
    const BookEntry* found = entry(stock_locate);
    return found != nullptr ? &found->book : nullptr;
}

auto BookManager::book_for_symbol(std::string_view symbol) const -> const L3Book* {
    for (const auto& slot : m_books_by_locate) {
        if (slot && slot->book.symbol() == symbol) {
            return &slot->book;
        }
    }
    return nullptr;
}

auto BookManager::symbol_for_locate(std::uint16_t stock_locate) const -> std::string_view {
    if (const BookEntry* found = entry(stock_locate);
        found != nullptr && !found->book.symbol().empty()) {
        return found->book.symbol();
    }
    if (stock_locate < m_symbol_by_locate.size()) {
        return m_symbol_by_locate[stock_locate];
    }
    return {};
}

}  // namespace itch::book
