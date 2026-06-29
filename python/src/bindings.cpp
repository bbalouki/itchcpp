#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <fstream>
#include <string>
#include <variant>
#include <vector>

#include "itch/analytics/vwap.hpp"
#include "itch/book/book_manager.hpp"
#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/parser.hpp"

namespace py = pybind11;

namespace {

// A thin parser facade mirroring the pure-Python `itch` package's MessageParser,
// so this native module can serve as a drop-in, faster backend for it.
struct MessageParser {};

// Standard price divisor (4 implied decimals); MWCB levels override this.
constexpr double STANDARD_DIVISOR = 10000.0;

// Binds the fields common to every ITCH message plus the shared helpers.
template <typename MsgType>
auto bind_common(py::class_<MsgType>& cls, double price_divisor = STANDARD_DIVISOR) -> void {
    cls.def_property_readonly("message_type", [](const MsgType& msg) {
        return std::string(1, msg.message_type);
    });
    cls.def_readonly("stock_locate", &MsgType::stock_locate);
    cls.def_readonly("tracking_number", &MsgType::tracking_number);
    cls.def_readonly("timestamp", &MsgType::timestamp);
    // decode_price(name) divides the named raw integer field by the scale, exactly
    // like the upstream package's MarketMessage.decode_price.
    cls.def(
        "decode_price",
        [price_divisor](const py::object& self, const std::string& name) {
            return self.attr(name.c_str()).cast<double>() / price_divisor;
        },
        py::arg("attribute_name")
    );
    // to_bytes() serializes the message back to wire form, matching the upstream
    // MarketMessage.to_bytes (here it returns the message body without the frame
    // length prefix).
    cls.def("to_bytes", [](const MsgType& msg) {
        const std::vector<std::byte> bytes = itch::encode_message(itch::Message {msg});
        return py::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());  // NOLINT
    });
}

// Exposes an 8-char stock symbol field as a trimmed Python string.
template <typename MsgType>
auto stock_property(py::class_<MsgType>& cls) -> void {
    cls.def_property_readonly("stock", [](const MsgType& msg) {
        return itch::to_string(msg.stock, itch::STOCK_LEN);
    });
}

auto parse_buffer(const std::string& data) -> py::list {
    py::list     out;
    itch::Parser parser;
    parser.parse(data.data(), data.size(), [&out](const itch::Message& message) {
        std::visit([&out](const auto& concrete) { out.append(py::cast(concrete)); }, message);
    });
    return out;
}

}  // namespace

PYBIND11_MODULE(itchcpp, mod) {
    mod.doc() =
        "Native (C++) NASDAQ TotalView-ITCH 5.0 parser. A faster drop-in backend "
        "for the pure-Python `itch` package: it exposes a MessageParser plus typed "
        "message classes with the same field and decode_price semantics.";

    // --- Message classes -----------------------------------------------------
    {
        py::class_<itch::SystemEventMessage> cls {mod, "SystemEventMessage"};
        bind_common(cls);
        cls.def_property_readonly("event_code", [](const itch::SystemEventMessage& m) {
            return std::string(1, m.event_code);
        });
    }
    {
        py::class_<itch::StockDirectoryMessage> cls {mod, "StockDirectoryMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly("round_lot_size", &itch::StockDirectoryMessage::round_lot_size);
        cls.def_property_readonly("market_category", [](const itch::StockDirectoryMessage& m) {
            return std::string(1, m.market_category);
        });
    }
    {
        py::class_<itch::StockTradingActionMessage> cls {mod, "StockTradingActionMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_property_readonly("trading_state", [](const itch::StockTradingActionMessage& m) {
            return std::string(1, m.trading_state);
        });
        cls.def_property_readonly("reason", [](const itch::StockTradingActionMessage& m) {
            return itch::to_string(m.reason, 4);
        });
    }
    {
        py::class_<itch::RegSHOMessage> cls {mod, "RegSHOMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_property_readonly("reg_sho_action", [](const itch::RegSHOMessage& m) {
            return std::string(1, m.reg_sho_action);
        });
    }
    {
        py::class_<itch::MarketParticipantPositionMessage> cls {
            mod, "MarketParticipantPositionMessage"
        };
        bind_common(cls);
        stock_property(cls);
        cls.def_property_readonly("mpid", [](const itch::MarketParticipantPositionMessage& m) {
            return itch::to_string(m.mpid, 4);
        });
    }
    {
        py::class_<itch::MWCBDeclineLevelMessage> cls {mod, "MWCBDeclineLevelMessage"};
        bind_common(cls, 1.0E8);  // MWCB levels carry 8 implied decimals.
        cls.def_readonly("level1", &itch::MWCBDeclineLevelMessage::level1);
        cls.def_readonly("level2", &itch::MWCBDeclineLevelMessage::level2);
        cls.def_readonly("level3", &itch::MWCBDeclineLevelMessage::level3);
    }
    {
        py::class_<itch::MWCBStatusMessage> cls {mod, "MWCBStatusMessage"};
        bind_common(cls);
        cls.def_property_readonly("breached_level", [](const itch::MWCBStatusMessage& m) {
            return std::string(1, m.breached_level);
        });
    }
    {
        py::class_<itch::IPOQuotingPeriodUpdateMessage> cls {mod, "IPOQuotingPeriodUpdateMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly("ipo_price", &itch::IPOQuotingPeriodUpdateMessage::ipo_price);
        cls.def_readonly(
            "ipo_quotation_release_time",
            &itch::IPOQuotingPeriodUpdateMessage::ipo_quotation_release_time
        );
    }
    {
        py::class_<itch::LULDAuctionCollarMessage> cls {mod, "LULDAuctionCollarMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly(
            "auction_collar_reference_price",
            &itch::LULDAuctionCollarMessage::auction_collar_reference_price
        );
        cls.def_readonly(
            "upper_auction_collar_price",
            &itch::LULDAuctionCollarMessage::upper_auction_collar_price
        );
        cls.def_readonly(
            "lower_auction_collar_price",
            &itch::LULDAuctionCollarMessage::lower_auction_collar_price
        );
    }
    {
        py::class_<itch::OperationalHaltMessage> cls {mod, "OperationalHaltMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_property_readonly("market_code", [](const itch::OperationalHaltMessage& m) {
            return std::string(1, m.market_code);
        });
        cls.def_property_readonly(
            "operational_halt_action", [](const itch::OperationalHaltMessage& m) {
                return std::string(1, m.operational_halt_action);
            }
        );
    }
    {
        py::class_<itch::AddOrderMessage> cls {mod, "AddOrderMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly("order_reference_number", &itch::AddOrderMessage::order_reference_number);
        cls.def_readonly("shares", &itch::AddOrderMessage::shares);
        cls.def_readonly("price", &itch::AddOrderMessage::price);
        cls.def_property_readonly("buy_sell_indicator", [](const itch::AddOrderMessage& m) {
            return std::string(1, m.buy_sell_indicator);
        });
    }
    {
        py::class_<itch::AddOrderMPIDAttributionMessage> cls {
            mod, "AddOrderMPIDAttributionMessage"
        };
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly(
            "order_reference_number", &itch::AddOrderMPIDAttributionMessage::order_reference_number
        );
        cls.def_readonly("shares", &itch::AddOrderMPIDAttributionMessage::shares);
        cls.def_readonly("price", &itch::AddOrderMPIDAttributionMessage::price);
        cls.def_property_readonly(
            "buy_sell_indicator", [](const itch::AddOrderMPIDAttributionMessage& m) {
                return std::string(1, m.buy_sell_indicator);
            }
        );
        cls.def_property_readonly("attribution", [](const itch::AddOrderMPIDAttributionMessage& m) {
            return itch::to_string(m.attribution, 4);
        });
    }
    {
        py::class_<itch::OrderExecutedMessage> cls {mod, "OrderExecutedMessage"};
        bind_common(cls);
        cls.def_readonly(
            "order_reference_number", &itch::OrderExecutedMessage::order_reference_number
        );
        cls.def_readonly("executed_shares", &itch::OrderExecutedMessage::executed_shares);
        cls.def_readonly("match_number", &itch::OrderExecutedMessage::match_number);
    }
    {
        py::class_<itch::OrderExecutedWithPriceMessage> cls {mod, "OrderExecutedWithPriceMessage"};
        bind_common(cls);
        cls.def_readonly(
            "order_reference_number", &itch::OrderExecutedWithPriceMessage::order_reference_number
        );
        cls.def_readonly("executed_shares", &itch::OrderExecutedWithPriceMessage::executed_shares);
        cls.def_readonly("match_number", &itch::OrderExecutedWithPriceMessage::match_number);
        cls.def_readonly("execution_price", &itch::OrderExecutedWithPriceMessage::execution_price);
        cls.def_property_readonly("printable", [](const itch::OrderExecutedWithPriceMessage& m) {
            return std::string(1, m.printable);
        });
    }
    {
        py::class_<itch::OrderCancelMessage> cls {mod, "OrderCancelMessage"};
        bind_common(cls);
        cls.def_readonly(
            "order_reference_number", &itch::OrderCancelMessage::order_reference_number
        );
        cls.def_readonly("cancelled_shares", &itch::OrderCancelMessage::cancelled_shares);
    }
    {
        py::class_<itch::OrderDeleteMessage> cls {mod, "OrderDeleteMessage"};
        bind_common(cls);
        cls.def_readonly(
            "order_reference_number", &itch::OrderDeleteMessage::order_reference_number
        );
    }
    {
        py::class_<itch::OrderReplaceMessage> cls {mod, "OrderReplaceMessage"};
        bind_common(cls);
        cls.def_readonly(
            "original_order_reference_number",
            &itch::OrderReplaceMessage::original_order_reference_number
        );
        cls.def_readonly(
            "new_order_reference_number", &itch::OrderReplaceMessage::new_order_reference_number
        );
        cls.def_readonly("shares", &itch::OrderReplaceMessage::shares);
        cls.def_readonly("price", &itch::OrderReplaceMessage::price);
    }
    {
        py::class_<itch::NonCrossTradeMessage> cls {mod, "NonCrossTradeMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly(
            "order_reference_number", &itch::NonCrossTradeMessage::order_reference_number
        );
        cls.def_readonly("shares", &itch::NonCrossTradeMessage::shares);
        cls.def_readonly("price", &itch::NonCrossTradeMessage::price);
        cls.def_readonly("match_number", &itch::NonCrossTradeMessage::match_number);
        cls.def_property_readonly("buy_sell_indicator", [](const itch::NonCrossTradeMessage& m) {
            return std::string(1, m.buy_sell_indicator);
        });
    }
    {
        py::class_<itch::CrossTradeMessage> cls {mod, "CrossTradeMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly("shares", &itch::CrossTradeMessage::shares);
        cls.def_readonly("cross_price", &itch::CrossTradeMessage::cross_price);
        cls.def_readonly("match_number", &itch::CrossTradeMessage::match_number);
        cls.def_property_readonly("cross_type", [](const itch::CrossTradeMessage& m) {
            return std::string(1, m.cross_type);
        });
    }
    {
        py::class_<itch::BrokenTradeMessage> cls {mod, "BrokenTradeMessage"};
        bind_common(cls);
        cls.def_readonly("match_number", &itch::BrokenTradeMessage::match_number);
    }
    {
        py::class_<itch::NOIIMessage> cls {mod, "NOIIMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly("paired_shares", &itch::NOIIMessage::paired_shares);
        cls.def_readonly("imbalance_shares", &itch::NOIIMessage::imbalance_shares);
        cls.def_readonly("far_price", &itch::NOIIMessage::far_price);
        cls.def_readonly("near_price", &itch::NOIIMessage::near_price);
        cls.def_readonly("current_reference_price", &itch::NOIIMessage::current_reference_price);
        cls.def_property_readonly("imbalance_direction", [](const itch::NOIIMessage& m) {
            return std::string(1, m.imbalance_direction);
        });
        cls.def_property_readonly("cross_type", [](const itch::NOIIMessage& m) {
            return std::string(1, m.cross_type);
        });
    }
    {
        py::class_<itch::RetailPriceImprovementIndicatorMessage> cls {
            mod, "RetailPriceImprovementIndicatorMessage"
        };
        bind_common(cls);
        stock_property(cls);
        cls.def_property_readonly(
            "interest_flag", [](const itch::RetailPriceImprovementIndicatorMessage& m) {
                return std::string(1, m.interest_flag);
            }
        );
    }
    {
        py::class_<itch::DLCRMessage> cls {mod, "DLCRMessage"};
        bind_common(cls);
        stock_property(cls);
        cls.def_readonly("near_execution_price", &itch::DLCRMessage::near_execution_price);
        cls.def_readonly("minimum_allowable_price", &itch::DLCRMessage::minimum_allowable_price);
        cls.def_readonly("maximum_allowable_price", &itch::DLCRMessage::maximum_allowable_price);
    }

    // --- Parser facade -------------------------------------------------------
    py::class_<MessageParser>(mod, "MessageParser")
        .def(py::init<>())
        .def(
            "parse_stream",
            [](MessageParser&, const py::bytes& data) { return parse_buffer(std::string {data}); },
            py::arg("raw_binary_data"),
            "Parse messages from raw bytes, returning a list of message objects."
        )
        .def(
            "parse_file",
            [](MessageParser&, const std::string& path) {
                std::ifstream     file {path, std::ios::binary};
                const std::string data {std::istreambuf_iterator<char> {file}, {}};
                return parse_buffer(data);
            },
            py::arg("itch_file"),
            "Read and parse all messages from a binary ITCH file path."
        );

    // --- Book engine ---------------------------------------------------------
    py::class_<itch::book::Bbo>(mod, "Bbo")
        .def_readonly("has_bid", &itch::book::Bbo::has_bid)
        .def_readonly("has_ask", &itch::book::Bbo::has_ask)
        .def_readonly("bid_shares", &itch::book::Bbo::bid_shares)
        .def_readonly("ask_shares", &itch::book::Bbo::ask_shares)
        .def_property_readonly(
            "bid_price", [](const itch::book::Bbo& b) { return b.bid_price.to_double(); }
        )
        .def_property_readonly("ask_price", [](const itch::book::Bbo& b) {
            return b.ask_price.to_double();
        });

    py::class_<itch::book::L3Book>(mod, "L3Book")
        .def_property_readonly("symbol", &itch::book::L3Book::symbol)
        .def("bbo", &itch::book::L3Book::bbo);

    py::class_<itch::book::BookManager>(mod, "BookManager")
        .def(py::init<>())
        .def("track_symbol", &itch::book::BookManager::track_symbol, py::arg("symbol"))
        .def(
            "process_stream",
            [](itch::book::BookManager& manager, const py::bytes& data) {
                const std::string buffer {data};
                itch::Parser      parser;
                parser.parse(buffer.data(), buffer.size(), [&manager](const itch::Message& msg) {
                    manager.process(msg);
                });
            },
            py::arg("raw_binary_data"),
            "Parse a raw ITCH buffer and update every symbol's book in one pass."
        )
        .def("book_count", &itch::book::BookManager::book_count)
        .def(
            "book_for_symbol",
            &itch::book::BookManager::book_for_symbol,
            py::arg("symbol"),
            py::return_value_policy::reference_internal
        );

    // --- Analytics -----------------------------------------------------------
    py::class_<itch::analytics::Vwap>(mod, "Vwap")
        .def(py::init<>())
        .def(
            "add",
            [](itch::analytics::Vwap& vwap, std::uint32_t raw_price, std::uint64_t shares) {
                vwap.add(itch::StandardPrice {raw_price}, shares);
            },
            py::arg("raw_price"),
            py::arg("shares")
        )
        .def("value", &itch::analytics::Vwap::value)
        .def("volume", &itch::analytics::Vwap::volume)
        .def("reset", &itch::analytics::Vwap::reset);
}
