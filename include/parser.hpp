#pragma once

#include <functional>
#include <istream>
#include <map>
#include <variant>
#include <vector>

#include "messages.hpp"

namespace itch {

/**
 The TotalView ITCH feed is composed of a series of messages that describe
 orders added to, removed from, and executed on Nasdaq as well as disseminate
 Cross and Stock Directory information.
 Each message begins with a one-byte Message Type field that identifies the
 structure of the remainder of the message.  The Message Type is followed by
 fields that are specific to each message type.

 All Message have the following attributes:
    - message_type: A single letter that identify the message
    - timestamp: Time at which the message was generated (Nanoseconds past
 midnight)
    - stock_locate: Locate code identifying the security
    - tracking_number: Nasdaq internal tracking number

    for more details on each message type, see the
  [documentation](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf).

 @note
 Prices are integers fields supplied with an associated precision.  When
 converted to a decimal format, prices are in fixed point format, where the
 precision defines the number of decimal places. For example, a field flagged as
 Price (4) has an implied 4 decimal places.  The maximum value of price (4) in
 TotalView ITCH is 200,000.0000 (decimal, 77359400 hex). ``price_precision`` is
 4 for all messages except MWCBDeclineLeveMessage where ``price_precision``
 is 8.
*/
using Message = std::variant<
    SystemEventMessage, StockDirectoryMessage, StockTradingActionMessage,
    RegSHOMessage, MarketParticipantPositionMessage, MWCBDeclineLevelMessage,
    MWCBStatusMessage, IPOQuotingPeriodUpdateMessage, LULDAuctionCollarMessage,
    OperationalHaltMessage, AddOrderMessage, AddOrderMPIDAttributionMessage,
    OrderExecutedMessage, OrderExecutedWithPriceMessage, OrderCancelMessage,
    OrderDeleteMessage, OrderReplaceMessage, NonCrossTradeMessage,
    CrossTradeMessage, BrokenTradeMessage, NOIIMessage,
    RetailPriceImprovementIndicatorMessage, DLCRMessage>;

/**
 * @brief The signature for the callback function used in the streaming
 * parse method.
 *
 * @param const Message& A const reference to the fully parsed message
 * object.
 */
using callback_t = std::function<void(const Message&)>;

/**
 * @brief A high-performance parser for the NASDAQ TotalView-ITCH 5.0 protocol.
 *
 * This class is designed to read a raw binary stream of ITCH 5.0 messages,
 * handle the message framing (based on the 2-byte length prefix), and
 * deserialize the byte payloads into structured, type-safe C++ objects.
 *
 * The parser can be used in two main ways:
 * 1. A callback-based approach for maximum memory efficiency, processing one
 *    message at a time as it is read from the stream.
 * 2. A vector-based approach that parses the entire stream and returns a
 *    collection of all messages, which is more convenient but uses more memory.
 *
 * @note This parser assumes the input stream is a raw, sequenced ITCH 5.0 feed
 *       without any higher-level protocol framing (e.g., SoupBinTCP packet
 * headers).
 */
class Parser {
   public:
    /**
     * @brief Constructs a Parser instance.
     *
     * The constructor pre-populates an internal dispatch table (a map of
     * handlers) by registering a specific parsing function for each known ITCH
     * message type. This setup makes the parsing process a fast lookup
     * operation at runtime.
     */
    Parser();

    /**
     * @brief Parses messages from a stream and invokes a callback for each one.
     *
     * This is the core, high-performance parsing method. It reads from the
     * stream sequentially, identifies each message, parses its payload, and
     * invokes the provided callback with the message type and the parsed
     * object. This approach is highly memory-efficient as it does not store
     * messages.
     *
     * @param is A reference to an std::istream opened in binary mode. The
     * stream's read position will be advanced as messages are consumed.
     * @param callback A function to be called for each successfully parsed
     * message.
     * @throw std::runtime_error if it fails to read from the stream or if the
     *        stream ends unexpectedly in the middle of a message.
     */
    void parse(std::istream& is, const callback_t& callback);

    /**
     * @brief Parses all messages from a stream and returns them in a vector.
     *
     * This is a convenience method that reads the entire stream and collects
     * all parsed messages into a single vector.
     *
     * @param is A reference to an std::istream opened in binary mode.
     * @return A std::vector<Message> containing all parsed messages.
     * @throw std::runtime_error on stream reading errors.
     * @note Be cautious with very large files, as this method will load all
     *       parsed messages into memory at once.
     */
    std::vector<Message> parse(std::istream& is);

    /**
     * @brief Parses messages from a stream, returning only those of specified
     * types.
     *
     * This convenience method reads the entire stream but only collects
     * messages whose type character is present in the `messages` filter list.
     *
     * @param is A reference to an std::istream opened in binary mode.
     * @param messages A vector of message type characters to keep (e.g., {'A',
     * 'E', 'P'}).
     * @return A std::vector<Message> containing only the filtered messages.
     * @throw std::runtime_error on stream reading errors.
     * @note Be cautious with very large files, as this method will load all
     *       matching messages into memory. The filtering is efficient.
     */
    std::vector<Message> parse(std::istream& is,
                               const std::vector<char>& messages);

   private:
    using Handler = std::function<Message(const char*)>;
    std::map<char, Handler> m_handlers;

    /**
     * @brief A template function to register a handler for a specific message type.
     * @tparam T The C++ struct type representing the message (e.g.,
     * AddOrderMessage).
     * @param type The character identifier for this message type (e.g., 'A').
     */
    template <typename T>
    void register_handler(char type);
};

}  // namespace itch
