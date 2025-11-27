#include "itch/order_book.hpp"

namespace itch {

// PriceLevel implementation
void PriceLevel::add_order(std::shared_ptr<Order> order) {
    total_shares += order->shares;
    orders.push_back(order);
    order->level = this;
}

void PriceLevel::remove_order(std::list<std::shared_ptr<Order>>::iterator order_it) {
    orders.erase(order_it);
}

// LimitOrderBook implementation
void LimitOrderBook::process(const Message& message) {
    std::visit([this](auto&& msg) { this->handle_message(msg); }, message);
}

void LimitOrderBook::print(std::ostream& out) const {
    out << "BIDS:\n";
    for (const auto& [price, level] : m_bids) {
        out << "  " << std::setw(10) << price << " : " << level.total_shares << "\n";
    }
    out << "ASKS:\n";
    for (const auto& [price, level] : m_asks) {
        out << "  " << std::setw(10) << price << " : " << level.total_shares << "\n";
    }
}

void LimitOrderBook::handle_message(const AddOrderMessage& msg) {
    add_order(msg.order_reference_number, msg.buy_sell_indicator, msg.shares, msg.price);
}

void LimitOrderBook::handle_message(const AddOrderMPIDAttributionMessage& msg) {
    add_order(msg.order_reference_number, msg.buy_sell_indicator, msg.shares, msg.price);
}

void LimitOrderBook::handle_message(const OrderExecutedMessage& msg) {
    remove_order(msg.order_reference_number, msg.executed_shares);
}

void LimitOrderBook::handle_message(const OrderExecutedWithPriceMessage& msg) {
    remove_order(msg.order_reference_number, msg.executed_shares);
}

void LimitOrderBook::handle_message(const OrderCancelMessage& msg) {
    remove_order(msg.order_reference_number, msg.cancelled_shares);
}

void LimitOrderBook::handle_message(const OrderDeleteMessage& msg) {
    auto it = m_orders.find(msg.order_reference_number);
    if (it != m_orders.end()) {
        auto order = *it->second;
        remove_order(msg.order_reference_number, order->shares);
    }
}

void LimitOrderBook::handle_message(const OrderReplaceMessage& msg) {
    auto it = m_orders.find(msg.original_order_reference_number);
    if (it != m_orders.end()) {
        auto old_order = *it->second;
        char side = old_order->buy_sell_indicator;
        remove_order(msg.original_order_reference_number, old_order->shares);
        add_order(msg.new_order_reference_number, side, msg.shares, msg.price);
    }
}

void LimitOrderBook::add_order(uint64_t order_ref, char side, uint32_t shares, uint32_t price) {
    auto order = std::make_shared<Order>(order_ref, side, shares, price);

    if (side == 'B') {
        auto& level = m_bids[price];
        level.add_order(order);
        m_orders[order_ref] = std::prev(level.orders.end());
    } else if (side == 'S') {
        auto& level = m_asks[price];
        level.add_order(order);
        m_orders[order_ref] = std::prev(level.orders.end());
    }
}

void LimitOrderBook::remove_order(uint64_t order_ref, uint32_t canceled_shares) {
    auto it = m_orders.find(order_ref);
    if (it != m_orders.end()) {
        auto order_it = it->second;
        auto order = *order_it;

        PriceLevel* level = order->level;
        level->total_shares -= canceled_shares;

        order->shares -= canceled_shares;
        if (order->shares == 0) {
            level->remove_order(order_it);
            if (level->orders.empty()) {
                if (order->buy_sell_indicator == 'B') {
                    m_bids.erase(order->price);
                } else {
                    m_asks.erase(order->price);
                }
            }
            m_orders.erase(it);
        }
    }
}

}  // namespace itch
