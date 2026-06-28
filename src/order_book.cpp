#include "itch/order_book.hpp"

#include <chrono>
#include <format>
#include <string_view>
#include <thread>
#include <type_traits>

#include "itch/price.hpp"

namespace itch {

auto PriceLevel::add_order(const std::shared_ptr<Order>& order) -> void {
    total_shares += order->shares;
    orders.push_back(order);
    order->level = this;
}

auto PriceLevel::remove_order(const OrderIt& order_it) -> void { orders.erase(order_it); }

auto LimitOrderBook::process(const Message& message) -> void {
    std::visit(
        [this](auto&& msg) {
            if (!book_messages.contains(msg.message_type)) {
                return;
            }
            this->handle_message(msg);
        },
        message
    );
}

auto LimitOrderBook::print(std::ostream& out, unsigned int delay_ms) const -> void {
    constexpr std::string_view BORDER   = "==========================================\n";
    constexpr std::string_view MID_RULE = "-----------+--------------+--------------\n";

    out << BORDER;
    out << "   SHARES  |    PRICE     | SIDE \n";
    out << BORDER;

    const auto print_level =
        [&](std::uint32_t total_shares, std::uint32_t raw_price, std::string_view side) {
            if (delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            out << std::format(
                "{:>10} | {:>12.4f} | {}\n", total_shares, make_price(raw_price).to_double(), side
            );
            if (delay_ms > 0) {
                out << std::flush;
            }
        };

    // Asks are stored low-to-high; print them high-to-low so the best ask sits
    // just above the spread.
    for (auto iter = m_asks.rbegin(); iter != m_asks.rend(); ++iter) {
        print_level(iter->second.total_shares, iter->first, "Ask");
    }

    out << MID_RULE;

    // Bids are stored high-to-low, which is already the desired print order.
    for (const auto& [raw_price, level] : m_bids) {
        print_level(level.total_shares, raw_price, "Bid");
    }

    out << BORDER;
}

auto LimitOrderBook::handle_message(const AddOrderMessage& msg) -> void {
    if (to_string(msg.stock, sizeof(msg.stock)) != m_stock_symbol) {
        return;
    }
    add_order(
        msg.order_reference_number,
        msg.buy_sell_indicator,
        msg.shares,
        msg.price,
        to_string(msg.stock, sizeof(msg.stock))
    );
}

auto LimitOrderBook::handle_message(const AddOrderMPIDAttributionMessage& msg) -> void {
    if (to_string(msg.stock, sizeof(msg.stock)) != m_stock_symbol) {
        return;
    }
    add_order(
        msg.order_reference_number,
        msg.buy_sell_indicator,
        msg.shares,
        msg.price,
        to_string(msg.stock, sizeof(msg.stock))
    );
}

auto LimitOrderBook::handle_message(const OrderExecutedMessage& msg) -> void {
    auto order_iter = m_orders.find(msg.order_reference_number);
    if (order_iter == m_orders.end() || (*order_iter->second)->stock != m_stock_symbol) {
        return;
    }
    remove_order(msg.order_reference_number, msg.executed_shares);
}

auto LimitOrderBook::handle_message(const OrderExecutedWithPriceMessage& msg) -> void {
    auto order_iter = m_orders.find(msg.order_reference_number);
    if (order_iter == m_orders.end() || (*order_iter->second)->stock != m_stock_symbol) {
        return;
    }
    remove_order(msg.order_reference_number, msg.executed_shares);
}

auto LimitOrderBook::handle_message(const OrderCancelMessage& msg) -> void {
    auto order_iter = m_orders.find(msg.order_reference_number);
    if (order_iter == m_orders.end() || (*order_iter->second)->stock != m_stock_symbol) {
        return;
    }
    remove_order(msg.order_reference_number, msg.cancelled_shares);
}

auto LimitOrderBook::handle_message(const OrderDeleteMessage& msg) -> void {
    auto iter = m_orders.find(msg.order_reference_number);
    if (iter != m_orders.end() && (*iter->second)->stock == m_stock_symbol) {
        auto order = *iter->second;
        remove_order(msg.order_reference_number, order->shares);
    }
}

auto LimitOrderBook::handle_message(const OrderReplaceMessage& msg) -> void {
    auto iter = m_orders.find(msg.original_order_reference_number);
    if (iter != m_orders.end() && (*iter->second)->stock == m_stock_symbol) {
        auto               old_order = *iter->second;
        char               side      = old_order->buy_sell_indicator;
        const std::string& stock     = old_order->stock;
        remove_order(msg.original_order_reference_number, old_order->shares);
        add_order(msg.new_order_reference_number, side, msg.shares, msg.price, stock);
    }
}

auto LimitOrderBook::add_order(
    uint64_t order_ref, char side, uint32_t shares, uint32_t price, const std::string& stock
) -> void {
    auto order = std::make_shared<Order>(order_ref, side, shares, price, stock);

    PriceLevel* level = nullptr;

    if (side == 'B') {
        level = &m_bids[price];
    } else {
        level = &m_asks[price];
    }
    order->level = level;
    level->add_order(order);
    m_orders[order_ref] = std::prev(level->orders.end());
}

auto LimitOrderBook::remove_order(uint64_t order_ref, uint32_t canceled_shares) -> void {
    auto iter = m_orders.find(order_ref);
    if (iter == m_orders.end()) {
        return;
    }

    auto order_iterator = iter->second;
    // This copy is deliberate: it keeps the Order alive after the list node is
    // erased below, since the order's price and side are read afterwards.
    auto        order_ptr = *order_iterator;  // NOLINT(performance-unnecessary-copy-initialization)
    PriceLevel* level     = order_ptr->level;

    level->total_shares -= canceled_shares;
    order_ptr->shares -= canceled_shares;

    if (order_ptr->shares == 0) {
        level->remove_order(order_iterator);
        if (level->orders.empty()) {
            if (order_ptr->buy_sell_indicator == 'B') {
                m_bids.erase(order_ptr->price);
            } else {
                m_asks.erase(order_ptr->price);
            }
        }
        m_orders.erase(iter);
    }
}

}  // namespace itch
