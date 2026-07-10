// Native (C++) backend for the pure-Python `itch` (PyPI: `itchfeed`) package.
//
// This module, compiled as `itchcpp._itchcpp`, mirrors the upstream public API
// exactly so it can serve as a faster drop-in: the same message classes (same
// names, same raw `bytes`/`int` attributes), the same `MessageParser` semantics
// (a `message_type` filter, lazy iteration, stop on the end-of-messages system
// event), the same `MarketMessage` helpers (`decode`, `decode_price`,
// `get_attributes`, `set_timestamp`, `split_timestamp`, `to_bytes`), and the same
// module members (`AllMessages`, `messages`, `create_message`). The pure-Python
// `itchcpp` package layered on top re-exports these under `itchcpp.messages`,
// `itchcpp.parser`, and `itchcpp.indicators`.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "itch/analytics/vwap.hpp"
#include "itch/book/book_manager.hpp"
#include "itch/encoder.hpp"
#include "itch/messages.hpp"
#include "itch/parser.hpp"

namespace py = pybind11;

namespace {

// The set of every known message-type byte, in the upstream order. Mirrors
// `itch.messages.AllMessages` and doubles as the default parser filter.
constexpr std::string_view ALL_MESSAGES = "SAFECXDUBHRYPQINLVWKJhO";

// The width of the 2-byte big-endian length prefix that frames each message.
constexpr std::size_t FRAME_HEADER_LEN = 2;

// Default chunk size for reading a file-backed stream (matches upstream's 64 KiB).
constexpr std::size_t DEFAULT_CACHE_SIZE = 65536;

//  Generic message helpers
// These operate on the Python object (`self`) so a single implementation serves
// every message class, exactly replicating the upstream `MarketMessage` methods.

// Returns whether a looked-up attribute value is callable (a bound method).
auto is_callable(const py::object& value) -> bool { return PyCallable_Check(value.ptr()) != 0; }

// decode_price(name): divide the named raw integer field by 10**price_precision,
// matching `MarketMessage.decode_price`. Raises ValueError for a method name and
// (via Python) TypeError when the attribute is not numeric.
auto message_decode_price(const py::object& self, const std::string& price_attr) -> py::object {
    const py::object value = self.attr(price_attr.c_str());
    if (is_callable(value)) {
        throw py::value_error("Please check the price attribute for " + price_attr);
    }
    const py::object precision = self.attr("price_precision");
    const py::object divisor   = py::int_(10).attr("__pow__")(precision);
    return value / divisor;
}

// set_timestamp(ts1, ts2): reconstruct the 48-bit timestamp from two 32-bit halves.
auto message_set_timestamp(const py::object& self, std::uint64_t ts1, std::uint64_t ts2) -> void {
    self.attr("timestamp") = py::int_(ts2 | (ts1 << 32U));
}

// split_timestamp(): split the timestamp into (high 32 bits, low 32 bits).
auto message_split_timestamp(const py::object& self) -> py::tuple {
    const auto          timestamp = self.attr("timestamp").cast<std::uint64_t>();
    const std::uint64_t high {timestamp >> 32U};
    const std::uint64_t low {timestamp - (high << 32U)};
    return py::make_tuple(high, low);
}

// get_attributes(call_able): the non-callable (data) or callable (method) public
// attribute names, derived from dir(self) just like the upstream method.
auto message_get_attributes(const py::object& self, bool call_able) -> py::list {
    py::list         out;
    const py::object names = py::module_::import("builtins").attr("dir")(self);
    for (const auto& name : names) {
        const auto text = name.cast<std::string>();
        if (text.rfind("__", 0) == 0) {
            continue;
        }
        if (is_callable(self.attr(name)) == call_able) {
            out.append(name);
        }
    }
    return out;
}

// decode(prefix): build a human-readable dataclass named `{prefix}{ClassName}`,
// turning bytes fields into rstripped ASCII strings and price fields into floats.
auto message_decode(const py::object& self, const std::string& prefix) -> py::object {
    const py::object builtins = py::module_::import("builtins");
    py::dict         attrs;
    for (const auto& name : builtins.attr("dir")(self)) {
        const auto text = name.cast<std::string>();
        if (text.rfind("__", 0) == 0) {
            continue;
        }
        py::object value = self.attr(name);
        if (text.find("price") != std::string::npos && text != "price_precision" &&
            text != "decode_price") {
            value = self.attr("decode_price")(name);
        }
        if (is_callable(value)) {
            continue;
        }
        if (py::isinstance<py::bytes>(value)) {
            attrs[name] = value.attr("decode")("ascii").attr("rstrip")();
        } else if (
            py::isinstance<py::int_>(value) || py::isinstance<py::float_>(value) ||
            py::isinstance<py::str>(value) || py::isinstance<py::bool_>(value)
        ) {
            attrs[name] = value;
        }
    }

    py::list fields;
    for (const auto& item : attrs) {
        fields.append(py::make_tuple(item.first, builtins.attr("type")(item.second)));
    }
    const auto class_name = prefix + self.attr("__class__").attr("__name__").cast<std::string>();
    const py::object make_dataclass = py::module_::import("dataclasses").attr("make_dataclass");
    return make_dataclass(class_name, fields)(**attrs);
}

//  Field exposure helpers

// Exposes a single-character field as a length-1 `bytes` value (read/write).
template <typename MsgType>
auto def_byte(py::class_<MsgType>& cls, const char* name, char MsgType::* member) -> void {
    cls.def_property(
        name,
        [member](const MsgType& msg) { return py::bytes(&(msg.*member), 1); },
        [member](MsgType& msg, const py::bytes& value) {
            const std::string text {value};
            msg.*member = text.empty() ? '\0' : text.front();
        }
    );
}

// Exposes a fixed-width character array as a `bytes` value of that width (the raw,
// space-padded symbol, not trimmed), matching upstream attribute semantics.
template <typename MsgType, std::size_t Width>
auto def_byte_array(py::class_<MsgType>& cls, const char* name, char (MsgType::*member)[Width])
    -> void {
    cls.def_property(
        name,
        [member](const MsgType& msg) { return py::bytes(msg.*member, Width); },
        [member](MsgType& msg, const py::bytes& value) {
            const std::string text {value};
            std::fill(std::begin(msg.*member), std::end(msg.*member), '\0');
            std::memcpy(msg.*member, text.data(), std::min(text.size(), Width));
        }
    );
}

// Serializes a message back to its wire body (type byte first, no frame length).
template <typename MsgType>
auto message_to_bytes(const MsgType& msg) -> py::bytes {
    const std::vector<std::byte> bytes = itch::encode_message(itch::Message {msg});
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};  // NOLINT
}

// Frames a body so the core parser (which expects a 2-byte big-endian length) can
// decode it: for every real ITCH message (< 256 bytes) this is byte-identical to
// the upstream `\x00` + length-byte header.
auto unpack_body(const std::string& body) -> itch::Message {
    std::string frame;
    frame.reserve(FRAME_HEADER_LEN + body.size());
    frame.push_back(static_cast<char>((body.size() >> 8U) & 0xFFU));
    frame.push_back(static_cast<char>(body.size() & 0xFFU));
    frame.append(body);

    itch::Parser                 parser;
    std::optional<itch::Message> result;
    parser.parse(frame.data(), frame.size(), [&result](const itch::Message& message) {
        result = message;
    });
    if (!result.has_value()) {
        throw py::value_error("Could not unpack message body");
    }
    return *result;
}

// Casts a parsed variant alternative to its concrete Python message object.
auto message_to_object(const itch::Message& message) -> py::object {
    return std::visit([](const auto& concrete) { return py::cast(concrete); }, message);
}

// Binds the fields and helpers common to every message class. `precision` is 4 for
// all messages except the MWCB decline levels (8). Returns the class so the caller
// can append its message-specific fields.
template <typename MsgType>
auto bind_common(py::module_& mod, const char* name, const char* description, int precision)
    -> py::class_<MsgType> {
    py::class_<MsgType> cls {mod, name};
    cls.def(py::init<>());
    cls.def(py::init([](const py::bytes& body) {
        return std::get<MsgType>(unpack_body(std::string {body}));
    }));

    cls.def_property_readonly("message_type", [](const MsgType& msg) {
        return py::bytes(&msg.message_type, 1);
    });
    cls.def_readwrite("stock_locate", &MsgType::stock_locate);
    cls.def_readwrite("tracking_number", &MsgType::tracking_number);
    cls.def_readwrite("timestamp", &MsgType::timestamp);

    cls.def("decode_price", &message_decode_price, py::arg("price_attr"));
    cls.def("set_timestamp", &message_set_timestamp, py::arg("ts1"), py::arg("ts2"));
    cls.def("split_timestamp", &message_split_timestamp);
    cls.def("get_attributes", &message_get_attributes, py::arg("call_able") = false);
    cls.def("decode", &message_decode, py::arg("prefix") = std::string {});
    cls.def("to_bytes", &message_to_bytes<MsgType>);
    cls.def("__bytes__", &message_to_bytes<MsgType>);
    cls.def("__repr__", [](const py::object& self) { return py::repr(self.attr("decode")()); });

    cls.attr("description")     = py::str(description);
    cls.attr("price_precision") = py::int_(precision);
    cls.attr("message_size")    = py::int_(itch::encode_message(itch::Message {MsgType {}}).size());
    return cls;
}

constexpr int STANDARD_PRECISION = 4;
constexpr int MWCB_PRECISION     = 8;

// A factory that default-constructs each message type for `create_message`.
using Factory = std::function<py::object()>;

}  // namespace

PYBIND11_MODULE(_itchcpp, mod) {
    mod.doc() =
        "Native (C++) NASDAQ TotalView-ITCH 5.0 backend for the pure-Python `itch` "
        "(itchfeed) package: a MessageParser plus typed message classes with matching "
        "raw attributes and decode/decode_price/to_bytes semantics.";

    // Maps a message-type byte to its Python class and to a default-constructing
    // factory, powering the module-level `messages` dict and `create_message`.
    auto type_to_class   = std::make_shared<py::dict>();
    auto type_to_factory = std::make_shared<std::map<char, Factory>>();

    const auto register_message = [&](char type_byte, const py::object& cls, Factory factory) {
        (*type_to_class)[py::bytes(&type_byte, 1)] = cls;
        (*type_to_factory)[type_byte]              = std::move(factory);
    };

    //  Message classes
    {
        auto cls = bind_common<itch::SystemEventMessage>(
            mod, "SystemEventMessage", "System Event Message", STANDARD_PRECISION
        );
        def_byte(cls, "event_code", &itch::SystemEventMessage::event_code);
        register_message('S', cls, [] { return py::cast(itch::SystemEventMessage {}); });
    }
    {
        auto cls = bind_common<itch::StockDirectoryMessage>(
            mod, "StockDirectoryMessage", "Stock Directory Message", STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::StockDirectoryMessage::stock);
        def_byte(cls, "market_category", &itch::StockDirectoryMessage::market_category);
        def_byte(
            cls,
            "financial_status_indicator",
            &itch::StockDirectoryMessage::financial_status_indicator
        );
        cls.def_readwrite("round_lot_size", &itch::StockDirectoryMessage::round_lot_size);
        def_byte(cls, "round_lots_only", &itch::StockDirectoryMessage::round_lots_only);
        def_byte(cls, "issue_classification", &itch::StockDirectoryMessage::issue_classification);
        def_byte_array(cls, "issue_sub_type", &itch::StockDirectoryMessage::issue_sub_type);
        def_byte(cls, "authenticity", &itch::StockDirectoryMessage::authenticity);
        def_byte(
            cls,
            "short_sale_threshold_indicator",
            &itch::StockDirectoryMessage::short_sale_threshold_indicator
        );
        def_byte(cls, "ipo_flag", &itch::StockDirectoryMessage::ipo_flag);
        def_byte(cls, "luld_ref", &itch::StockDirectoryMessage::luld_ref);
        def_byte(cls, "etp_flag", &itch::StockDirectoryMessage::etp_flag);
        cls.def_readwrite("etp_leverage_factor", &itch::StockDirectoryMessage::etp_leverage_factor);
        def_byte(cls, "inverse_indicator", &itch::StockDirectoryMessage::inverse_indicator);
        register_message('R', cls, [] { return py::cast(itch::StockDirectoryMessage {}); });
    }
    {
        auto cls = bind_common<itch::StockTradingActionMessage>(
            mod, "StockTradingActionMessage", "Stock Trading Action Message", STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::StockTradingActionMessage::stock);
        def_byte(cls, "trading_state", &itch::StockTradingActionMessage::trading_state);
        def_byte(cls, "reserved", &itch::StockTradingActionMessage::reserved);
        def_byte_array(cls, "reason", &itch::StockTradingActionMessage::reason);
        register_message('H', cls, [] { return py::cast(itch::StockTradingActionMessage {}); });
    }
    {
        auto cls = bind_common<itch::RegSHOMessage>(
            mod,
            "RegSHOMessage",
            "Reg SHO Short Sale Price Test Restricted Indicator",
            STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::RegSHOMessage::stock);
        def_byte(cls, "reg_sho_action", &itch::RegSHOMessage::reg_sho_action);
        register_message('Y', cls, [] { return py::cast(itch::RegSHOMessage {}); });
    }
    {
        auto cls = bind_common<itch::MarketParticipantPositionMessage>(
            mod,
            "MarketParticipantPositionMessage",
            "Market Participant Position message",
            STANDARD_PRECISION
        );
        def_byte_array(cls, "mpid", &itch::MarketParticipantPositionMessage::mpid);
        def_byte_array(cls, "stock", &itch::MarketParticipantPositionMessage::stock);
        def_byte(
            cls,
            "primary_market_maker",
            &itch::MarketParticipantPositionMessage::primary_market_maker
        );
        def_byte(
            cls, "market_maker_mode", &itch::MarketParticipantPositionMessage::market_maker_mode
        );
        def_byte(
            cls,
            "market_participant_state",
            &itch::MarketParticipantPositionMessage::market_participant_state
        );
        register_message('L', cls, [] {
            return py::cast(itch::MarketParticipantPositionMessage {});
        });
    }
    {
        // Upstream spells this class `MWCBDeclineLeveMessage` (sic) and names the
        // levels `levelN_price`; mirror both for drop-in compatibility.
        auto cls = bind_common<itch::MWCBDeclineLevelMessage>(
            mod,
            "MWCBDeclineLeveMessage",
            "Market wide circuit breaker Decline Level Message",
            MWCB_PRECISION
        );
        cls.def_readwrite("level1_price", &itch::MWCBDeclineLevelMessage::level1);
        cls.def_readwrite("level2_price", &itch::MWCBDeclineLevelMessage::level2);
        cls.def_readwrite("level3_price", &itch::MWCBDeclineLevelMessage::level3);
        register_message('V', cls, [] { return py::cast(itch::MWCBDeclineLevelMessage {}); });
    }
    {
        auto cls = bind_common<itch::MWCBStatusMessage>(
            mod,
            "MWCBStatusMessage",
            "Market-Wide Circuit Breaker Status message",
            STANDARD_PRECISION
        );
        def_byte(cls, "breached_level", &itch::MWCBStatusMessage::breached_level);
        register_message('W', cls, [] { return py::cast(itch::MWCBStatusMessage {}); });
    }
    {
        auto cls = bind_common<itch::IPOQuotingPeriodUpdateMessage>(
            mod,
            "IPOQuotingPeriodUpdateMessage",
            "IPO Quoting Period Update Message",
            STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::IPOQuotingPeriodUpdateMessage::stock);
        cls.def_readwrite(
            "ipo_release_time", &itch::IPOQuotingPeriodUpdateMessage::ipo_quotation_release_time
        );
        def_byte(
            cls,
            "ipo_release_qualifier",
            &itch::IPOQuotingPeriodUpdateMessage::ipo_quotation_release_qualifier
        );
        cls.def_readwrite("ipo_price", &itch::IPOQuotingPeriodUpdateMessage::ipo_price);
        register_message('K', cls, [] { return py::cast(itch::IPOQuotingPeriodUpdateMessage {}); });
    }
    {
        auto cls = bind_common<itch::LULDAuctionCollarMessage>(
            mod, "LULDAuctionCollarMessage", "LULD Auction Collar", STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::LULDAuctionCollarMessage::stock);
        cls.def_readwrite(
            "auction_collar_reference_price",
            &itch::LULDAuctionCollarMessage::auction_collar_reference_price
        );
        cls.def_readwrite(
            "upper_auction_collar_price",
            &itch::LULDAuctionCollarMessage::upper_auction_collar_price
        );
        cls.def_readwrite(
            "lower_auction_collar_price",
            &itch::LULDAuctionCollarMessage::lower_auction_collar_price
        );
        // Upstream attribute keeps the original `extention` spelling.
        cls.def_readwrite(
            "auction_collar_extention", &itch::LULDAuctionCollarMessage::auction_collar_extension
        );
        register_message('J', cls, [] { return py::cast(itch::LULDAuctionCollarMessage {}); });
    }
    {
        auto cls = bind_common<itch::OperationalHaltMessage>(
            mod, "OperationalHaltMessage", "Operational Halt", STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::OperationalHaltMessage::stock);
        def_byte(cls, "market_code", &itch::OperationalHaltMessage::market_code);
        def_byte(
            cls, "operational_halt_action", &itch::OperationalHaltMessage::operational_halt_action
        );
        register_message('h', cls, [] { return py::cast(itch::OperationalHaltMessage {}); });
    }
    {
        auto cls = bind_common<itch::AddOrderMessage>(
            mod,
            "AddOrderNoMPIAttributionMessage",
            "Add Order - No MPID Attribution Message",
            STANDARD_PRECISION
        );
        cls.def_readwrite("order_reference_number", &itch::AddOrderMessage::order_reference_number);
        def_byte(cls, "buy_sell_indicator", &itch::AddOrderMessage::buy_sell_indicator);
        cls.def_readwrite("shares", &itch::AddOrderMessage::shares);
        def_byte_array(cls, "stock", &itch::AddOrderMessage::stock);
        cls.def_readwrite("price", &itch::AddOrderMessage::price);
        register_message('A', cls, [] { return py::cast(itch::AddOrderMessage {}); });
    }
    {
        auto cls = bind_common<itch::AddOrderMPIDAttributionMessage>(
            mod,
            "AddOrderMPIDAttribution",
            "Add Order - MPID Attribution Message",
            STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::AddOrderMPIDAttributionMessage::order_reference_number
        );
        def_byte(
            cls, "buy_sell_indicator", &itch::AddOrderMPIDAttributionMessage::buy_sell_indicator
        );
        cls.def_readwrite("shares", &itch::AddOrderMPIDAttributionMessage::shares);
        def_byte_array(cls, "stock", &itch::AddOrderMPIDAttributionMessage::stock);
        cls.def_readwrite("price", &itch::AddOrderMPIDAttributionMessage::price);
        def_byte_array(cls, "attribution", &itch::AddOrderMPIDAttributionMessage::attribution);
        register_message('F', cls, [] {
            return py::cast(itch::AddOrderMPIDAttributionMessage {});
        });
    }
    {
        auto cls = bind_common<itch::OrderExecutedMessage>(
            mod, "OrderExecutedMessage", "Add Order - Order Executed Message", STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::OrderExecutedMessage::order_reference_number
        );
        cls.def_readwrite("executed_shares", &itch::OrderExecutedMessage::executed_shares);
        cls.def_readwrite("match_number", &itch::OrderExecutedMessage::match_number);
        register_message('E', cls, [] { return py::cast(itch::OrderExecutedMessage {}); });
    }
    {
        auto cls = bind_common<itch::OrderExecutedWithPriceMessage>(
            mod,
            "OrderExecutedWithPriceMessage",
            "Add Order - Order Executed with Price Message",
            STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::OrderExecutedWithPriceMessage::order_reference_number
        );
        cls.def_readwrite("executed_shares", &itch::OrderExecutedWithPriceMessage::executed_shares);
        cls.def_readwrite("match_number", &itch::OrderExecutedWithPriceMessage::match_number);
        def_byte(cls, "printable", &itch::OrderExecutedWithPriceMessage::printable);
        cls.def_readwrite("execution_price", &itch::OrderExecutedWithPriceMessage::execution_price);
        register_message('C', cls, [] { return py::cast(itch::OrderExecutedWithPriceMessage {}); });
    }
    {
        auto cls = bind_common<itch::OrderCancelMessage>(
            mod, "OrderCancelMessage", "Order Cancel Message", STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::OrderCancelMessage::order_reference_number
        );
        cls.def_readwrite("cancelled_shares", &itch::OrderCancelMessage::cancelled_shares);
        register_message('X', cls, [] { return py::cast(itch::OrderCancelMessage {}); });
    }
    {
        auto cls = bind_common<itch::OrderDeleteMessage>(
            mod, "OrderDeleteMessage", "Order Delete Message", STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::OrderDeleteMessage::order_reference_number
        );
        register_message('D', cls, [] { return py::cast(itch::OrderDeleteMessage {}); });
    }
    {
        auto cls = bind_common<itch::OrderReplaceMessage>(
            mod, "OrderReplaceMessage", "Order Replace Message", STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::OrderReplaceMessage::original_order_reference_number
        );
        cls.def_readwrite(
            "new_order_reference_number", &itch::OrderReplaceMessage::new_order_reference_number
        );
        cls.def_readwrite("shares", &itch::OrderReplaceMessage::shares);
        cls.def_readwrite("price", &itch::OrderReplaceMessage::price);
        register_message('U', cls, [] { return py::cast(itch::OrderReplaceMessage {}); });
    }
    {
        auto cls = bind_common<itch::NonCrossTradeMessage>(
            mod, "NonCrossTradeMessage", "Trade Message", STANDARD_PRECISION
        );
        cls.def_readwrite(
            "order_reference_number", &itch::NonCrossTradeMessage::order_reference_number
        );
        def_byte(cls, "buy_sell_indicator", &itch::NonCrossTradeMessage::buy_sell_indicator);
        cls.def_readwrite("shares", &itch::NonCrossTradeMessage::shares);
        def_byte_array(cls, "stock", &itch::NonCrossTradeMessage::stock);
        cls.def_readwrite("price", &itch::NonCrossTradeMessage::price);
        cls.def_readwrite("match_number", &itch::NonCrossTradeMessage::match_number);
        register_message('P', cls, [] { return py::cast(itch::NonCrossTradeMessage {}); });
    }
    {
        auto cls = bind_common<itch::CrossTradeMessage>(
            mod, "CrossTradeMessage", "Cross Trade Message", STANDARD_PRECISION
        );
        cls.def_readwrite("shares", &itch::CrossTradeMessage::shares);
        def_byte_array(cls, "stock", &itch::CrossTradeMessage::stock);
        cls.def_readwrite("cross_price", &itch::CrossTradeMessage::cross_price);
        cls.def_readwrite("match_number", &itch::CrossTradeMessage::match_number);
        def_byte(cls, "cross_type", &itch::CrossTradeMessage::cross_type);
        register_message('Q', cls, [] { return py::cast(itch::CrossTradeMessage {}); });
    }
    {
        auto cls = bind_common<itch::BrokenTradeMessage>(
            mod, "BrokenTradeMessage", "Broken Trade Message", STANDARD_PRECISION
        );
        cls.def_readwrite("match_number", &itch::BrokenTradeMessage::match_number);
        register_message('B', cls, [] { return py::cast(itch::BrokenTradeMessage {}); });
    }
    {
        auto cls =
            bind_common<itch::NOIIMessage>(mod, "NOIIMessage", "NOII Message", STANDARD_PRECISION);
        cls.def_readwrite("paired_shares", &itch::NOIIMessage::paired_shares);
        cls.def_readwrite("imbalance_shares", &itch::NOIIMessage::imbalance_shares);
        def_byte(cls, "imbalance_direction", &itch::NOIIMessage::imbalance_direction);
        def_byte_array(cls, "stock", &itch::NOIIMessage::stock);
        cls.def_readwrite("far_price", &itch::NOIIMessage::far_price);
        cls.def_readwrite("near_price", &itch::NOIIMessage::near_price);
        cls.def_readwrite("current_reference_price", &itch::NOIIMessage::current_reference_price);
        def_byte(cls, "cross_type", &itch::NOIIMessage::cross_type);
        def_byte(cls, "variation_indicator", &itch::NOIIMessage::price_variation_indicator);
        register_message('I', cls, [] { return py::cast(itch::NOIIMessage {}); });
    }
    {
        auto cls = bind_common<itch::RetailPriceImprovementIndicatorMessage>(
            mod, "RetailPriceImprovementIndicator", "Retail Interest message", STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::RetailPriceImprovementIndicatorMessage::stock);
        def_byte(
            cls, "interest_flag", &itch::RetailPriceImprovementIndicatorMessage::interest_flag
        );
        register_message('N', cls, [] {
            return py::cast(itch::RetailPriceImprovementIndicatorMessage {});
        });
    }
    {
        auto cls = bind_common<itch::DLCRMessage>(
            mod, "DLCRMessage", "Direct Listing with Capital Raise Message", STANDARD_PRECISION
        );
        def_byte_array(cls, "stock", &itch::DLCRMessage::stock);
        def_byte(cls, "open_eligibility_status", &itch::DLCRMessage::open_eligibility_status);
        cls.def_readwrite("minimum_allowable_price", &itch::DLCRMessage::minimum_allowable_price);
        cls.def_readwrite("maximum_allowable_price", &itch::DLCRMessage::maximum_allowable_price);
        cls.def_readwrite("near_execution_price", &itch::DLCRMessage::near_execution_price);
        cls.def_readwrite("near_execution_time", &itch::DLCRMessage::near_execution_time);
        cls.def_readwrite("lower_price_range_collar", &itch::DLCRMessage::lower_price_range_collar);
        cls.def_readwrite("upper_price_range_collar", &itch::DLCRMessage::upper_price_range_collar);
        register_message('O', cls, [] { return py::cast(itch::DLCRMessage {}); });
    }

    //  Module-level message registry and factory
    mod.attr("AllMessages") = py::bytes(std::string {ALL_MESSAGES});
    mod.attr("messages")    = *type_to_class;

    mod.def(
        "create_message",
        [type_to_factory](const py::bytes& message_type, const py::kwargs& kwargs) -> py::object {
            const std::string type_text {message_type};
            if (type_text.size() != 1) {
                throw py::value_error("message_type must be a single byte");
            }
            const auto factory = type_to_factory->find(type_text.front());
            if (factory == type_to_factory->end()) {
                throw py::value_error("Unknown message type: " + type_text);
            }
            py::object              instance = factory->second();
            constexpr std::uint64_t timestamp_mask {(std::uint64_t {1} << 48U) - 1U};
            for (const auto& item : kwargs) {
                if (item.first.cast<std::string>() == "timestamp") {
                    instance.attr("timestamp") =
                        py::int_(item.second.cast<std::uint64_t>() & timestamp_mask);
                } else {
                    instance.attr(item.first) = item.second;
                }
            }
            return instance;
        },
        py::arg("message_type"),
        "Create a message of the given type byte, populated from keyword attributes."
    );

    //  Parser 
    // A lazy iterator over framed ITCH messages, applying the type filter, the
    // optional save_file re-emission, and the stop-on-system-event-'C' rule. When
    // `m_file` is a binary file object the buffer is refilled `m_cachesize` bytes at
    // a time (consumed bytes are dropped), so a whole-day feed never has to be held
    // in memory at once; for an in-memory stream `m_file` is None.
    struct MessageIterator {
        std::string m_buffer;
        std::size_t m_offset {0};
        std::string m_filter;
        py::object  m_file;
        std::size_t m_cachesize {DEFAULT_CACHE_SIZE};
        py::object  m_save_file;
        bool        m_stopped {false};
    };

    py::class_<MessageIterator>(mod, "_MessageIterator")
        .def("__iter__", [](py::object self) { return self; })
        .def("__next__", [](MessageIterator& iter) -> py::object {
            // Ensures at least `need` unconsumed bytes are buffered, pulling more
            // from the file (and dropping already-consumed bytes) when possible.
            const auto fill_to = [&iter](std::size_t need) -> bool {
                while (iter.m_buffer.size() - iter.m_offset < need) {
                    if (iter.m_file.is_none()) {
                        return false;
                    }
                    if (iter.m_offset > 0) {
                        iter.m_buffer.erase(0, iter.m_offset);
                        iter.m_offset = 0;
                    }
                    const std::string chunk {
                        iter.m_file.attr("read")(iter.m_cachesize).cast<py::bytes>()
                    };
                    if (chunk.empty()) {
                        return false;
                    }
                    iter.m_buffer += chunk;
                }
                return true;
            };

            while (true) {
                if (iter.m_stopped || !fill_to(FRAME_HEADER_LEN)) {
                    throw py::stop_iteration();
                }
                const auto high = static_cast<unsigned char>(iter.m_buffer[iter.m_offset]);
                const auto low  = static_cast<unsigned char>(iter.m_buffer[iter.m_offset + 1]);
                const std::size_t body_len {(static_cast<std::size_t>(high) << 8U) | low};
                const std::size_t total {FRAME_HEADER_LEN + body_len};
                // fill_to may compact the buffer (resetting m_offset), so read the
                // frame position only after it returns.
                if (!fill_to(total)) {
                    throw py::stop_iteration();  // Incomplete trailing message.
                }
                const std::size_t pos = iter.m_offset;

                const char type_byte = iter.m_buffer[pos + FRAME_HEADER_LEN];
                if (ALL_MESSAGES.find(type_byte) == std::string_view::npos) {
                    throw py::value_error(std::string {"Unknown message type: "} + type_byte);
                }

                itch::Parser                 parser;
                std::optional<itch::Message> parsed;
                parser.parse(
                    iter.m_buffer.data() + pos, total, [&parsed](const itch::Message& message) {
                        parsed = message;
                    }
                );
                const std::string_view frame {iter.m_buffer.data() + pos, total};
                iter.m_offset = pos + total;

                const bool kept    = iter.m_filter.find(type_byte) != std::string::npos;
                const bool is_stop = type_byte == 'S' &&
                                     std::get<itch::SystemEventMessage>(*parsed).event_code == 'C';

                if (kept) {
                    if (!iter.m_save_file.is_none()) {
                        iter.m_save_file.attr("write")(py::bytes(frame.data(), frame.size()));
                    }
                    if (is_stop) {
                        iter.m_stopped = true;
                    }
                    return message_to_object(*parsed);
                }
                if (is_stop) {
                    throw py::stop_iteration();
                }
            }
        });

    struct MessageParser {
        std::string m_filter;
    };

    const auto make_iterator = [](const std::string& filter,
                                  std::string        buffer,
                                  py::object         file,
                                  std::size_t        cachesize,
                                  const py::object&  save_file) {
        MessageIterator iter;
        iter.m_buffer    = std::move(buffer);
        iter.m_filter    = filter;
        iter.m_file      = std::move(file);
        iter.m_cachesize = cachesize;
        iter.m_save_file = save_file;
        return iter;
    };

    py::class_<MessageParser>(mod, "MessageParser")
        .def(
            py::init([](const py::bytes& message_type) {
                return MessageParser {std::string {message_type}};
            }),
            py::arg("message_type") = py::bytes(std::string {ALL_MESSAGES})
        )
        .def_property_readonly(
            "message_type", [](const MessageParser& self) { return py::bytes(self.m_filter); }
        )
        .def(
            "get_message_type",
            [](const MessageParser&, const py::bytes& message) {
                const std::string body {message};
                if (body.empty() || ALL_MESSAGES.find(body.front()) == std::string_view::npos) {
                    throw py::value_error(
                        std::string {"Unknown message type: "} + (body.empty() ? '?' : body.front())
                    );
                }
                return message_to_object(unpack_body(body));
            },
            py::arg("message")
        )
        .def(
            "parse_stream",
            [make_iterator](
                const MessageParser& self, const py::bytes& data, const py::object& save_file
            ) {
                return make_iterator(
                    self.m_filter, std::string {data}, py::none(), DEFAULT_CACHE_SIZE, save_file
                );
            },
            py::arg("data"),
            py::arg("save_file") = py::none()
        )
        .def(
            "parse_file",
            [make_iterator](
                const MessageParser& self,
                const py::object&    file,
                std::size_t          cachesize,
                const py::object&    save_file
            ) {
                // The file is read lazily, `cachesize` bytes at a time, by the iterator.
                return make_iterator(self.m_filter, std::string {}, file, cachesize, save_file);
            },
            py::arg("file"),
            py::arg("cachesize") = DEFAULT_CACHE_SIZE,
            py::arg("save_file") = py::none()
        )
        .def(
            "parse_messages",
            [make_iterator](
                const MessageParser& self, const py::object& data, const py::object& callback
            ) {
                const bool is_buffer =
                    py::isinstance<py::bytes>(data) || py::isinstance<py::bytearray>(data);
                MessageIterator iter =
                    is_buffer
                        ? make_iterator(
                              self.m_filter,
                              std::string {py::bytes(data)},
                              py::none(),
                              DEFAULT_CACHE_SIZE,
                              py::none()
                          )
                        : make_iterator(
                              self.m_filter, std::string {}, data, DEFAULT_CACHE_SIZE, py::none()
                          );
                const py::object next = py::cast(iter).attr("__next__");
                while (true) {
                    py::object message;
                    try {
                        message = next();
                    } catch (py::error_already_set& stop) {
                        if (stop.matches(PyExc_StopIteration)) {
                            break;
                        }
                        throw;
                    }
                    callback(message);
                }
            },
            py::arg("data"),
            py::arg("callback")
        );

    //  Book engine (native extras, not part of upstream `itch`)
    py::class_<itch::book::Bbo>(mod, "Bbo")
        .def_readonly("has_bid", &itch::book::Bbo::has_bid)
        .def_readonly("has_ask", &itch::book::Bbo::has_ask)
        .def_readonly("bid_shares", &itch::book::Bbo::bid_shares)
        .def_readonly("ask_shares", &itch::book::Bbo::ask_shares)
        .def_property_readonly(
            "bid_price", [](const itch::book::Bbo& bbo) { return bbo.bid_price.to_double(); }
        )
        .def_property_readonly("ask_price", [](const itch::book::Bbo& bbo) {
            return bbo.ask_price.to_double();
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

    //  Analytics (native extra)
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
