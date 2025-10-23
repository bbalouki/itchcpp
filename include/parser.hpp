#pragma once

#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "itch.hpp"


namespace itch {

using Message = std::variant<
    SystemEventMessage, StockDirectoryMessage, StockTradingActionMessage,
    RegSHOMessage, MarketParticipantPositionMessage, MWCBDeclineLevelMessage,
    MWCBStatusMessage, IPOQuotingPeriodUpdateMessage, LULDAuctionCollarMessage,
    OperationalHaltMessage, AddOrderMessage, AddOrderMPIDAttributionMessage,
    OrderExecutedMessage, OrderExecutedWithPriceMessage, OrderCancelMessage,
    OrderDeleteMessage, OrderReplaceMessage, NonCrossTradeMessage,
    CrossTradeMessage, BrokenTradeMessage, NOIIMessage,
    RetailPriceImprovementIndicatorMessage, DLCRMessage>;

class Parser {
   public:
    Parser();
    void parse(std::istream& is,
               const std::function<void(const Message&)>& callback);
    std::vector<Message> parse(std::istream& is);

   private:
    using Handler = std::function<Message(const char*)>;
    std::map<char, Handler> m_handlers;
    std::map<char, size_t> m_message_sizes;

    template <typename T>
    void register_handler(char type);
};

}  // namespace itch
