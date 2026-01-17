#include "itch/order_book.hpp"

#include <chrono>
#include <thread>

namespace itch {

void PriceLevel::add_order(std::shared_ptr<Order> order) {
    total_shares += order->shares;
    orders.push_back(order);
    order->level = this;
}

void PriceLevel::remove_order(OrderIt order_it) { orders.erase(order_it); }

void LimitOrderBook::process(const Message& message) {
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

void LimitOrderBook::print(std::ostream& out, unsigned int delay_ms) const {
    std::ios state(nullptr);
    state.copyfmt(out);
    out << std::fixed << std::setprecision(4);

    out << "==========================================\n";
    out << "   SHARES  |    PRICE     | SIDE \n";
    out << "==========================================\n";

    if (!m_asks.empty()) {
        for (auto iter = m_asks.rbegin(); iter != m_asks.rend(); ++iter) {
            if (delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            double price = iter->first / PRICE_DIVISOR;
            out << std::setw(10) << iter->second.total_shares << " | " << std::setw(12) << price
                << " | Ask\n";
            if (delay_ms > 0)
                out << std::flush;
        }
    }

    out << "-----------+--------------+--------------\n";

    if (!m_bids.empty()) {
        for (const auto& [raw_price, level] : m_bids) {
            if (delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            double price = raw_price / PRICE_DIVISOR;
            out << std::setw(10) << level.total_shares << " | " << std::setw(12) << price
                << " | Bid\n";
            if (delay_ms > 0)
                out << std::flush;
        }
    }

    out << "==========================================\n";
    out.copyfmt(state);
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
    auto iter = m_orders.find(msg.order_reference_number);
    if (iter != m_orders.end()) {
        auto order = *iter->second;
        remove_order(msg.order_reference_number, order->shares);
    }
}

void LimitOrderBook::handle_message(const OrderReplaceMessage& msg) {
    auto iter = m_orders.find(msg.original_order_reference_number);
    if (iter != m_orders.end()) {
        auto old_order = *iter->second;
        char side      = old_order->buy_sell_indicator;
        remove_order(msg.original_order_reference_number, old_order->shares);
        add_order(msg.new_order_reference_number, side, msg.shares, msg.price);
    }
}

void LimitOrderBook::add_order(uint64_t order_ref, char side, uint32_t shares, uint32_t price) {
    auto order = std::make_shared<Order>(order_ref, side, shares, price);

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

void LimitOrderBook::remove_order(uint64_t order_ref, uint32_t canceled_shares) {
    auto iter = m_orders.find(order_ref);
    if (iter == m_orders.end()) {
        return;
    }

    OrderIt     order_iterator = iter->second;
    auto        order_ptr      = *order_iterator;
    PriceLevel* level          = order_ptr->level;

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
