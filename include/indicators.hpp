#pragma once

#include <map>
#include <string>

namespace itch {
namespace indicators {

const std::map<char, std::string> SYSTEM_EVENT_CODES = {
    {'O', "Start of Messages"},     {'S', "Start of System hours"},
    {'Q', "Start of Market hours"}, {'M', "End of Market hours"},
    {'E', "End of System hours"},   {'C', "End of Messages"},
};

const std::map<char, std::string> MARKET_CATEGORY = {
    {'N', "NYSE"},
    {'A', "AMEX"},
    {'P', "Arca"},
    {'Q', "NASDAQ Global Select"},
    {'G', "NASDAQ Global Market"},
    {'S', "NASDAQ Capital Market"},
    {'Z', "BATS"},
    {'V', "Investors Exchange"},
    {' ', "Not available"},
};

const std::map<char, std::string> FINANCIAL_STATUS_INDICATOR = {
    {'D', "Deficient"},
    {'E', "Delinquent"},
    {'Q', "Bankrupt"},
    {'S', "Suspended"},
    {'G', "Deficient and Bankrupt"},
    {'H', "Deficient and Delinquent"},
    {'J', "Delinquent and Bankrupt"},
    {'K', "Deficient, Delinquent and Bankrupt"},
    {'C', "Creations and/or Redemptions Suspended for Exchange Traded Product"},
    {'N', "Normal (Default): Issuer is NOT Deficient, Delinquent, or Bankrupt"},
    {' ', "Not available. Firms should refer to SIAC feeds for code if needed"},
};

const std::map<char, std::string> ISSUE_CLASSIFICATION_VALUES = {
    {'A', "American Depositary Share"},
    {'B', "Bond"},
    {'C', "Common Stock"},
    {'F', "Depository Receipt"},
    {'I', "144A"},
    {'L', "Limited Partnership"},
    {'N', "Notes"},
    {'O', "Ordinary Share"},
    {'P', "Preferred Stock"},
    {'Q', "Other Securities"},
    {'R', "Right"},
    {'S', "Shares of Beneficial Interest"},
    {'T', "Convertible Debenture"},
    {'U', "Unit"},
    {'V', "Units/Beneficial Interest"},
    {'W', "Warrant"},
};

const std::map<char, std::string> ISSUE_SUB_TYPE_VALUES = {

};
}  // namespace indicators

}  // namespace itch
