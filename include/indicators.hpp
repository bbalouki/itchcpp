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

const std::map<std::string, std::string> ISSUE_SUB_TYPE_VALUES = {
    {"A", "Preferred Trust Securities"},
    {"AI", "Alpha Index ETNs"},
    {"B", "Index Based Derivative"},
    {"C", "Common Shares"},
    {"CB", "Commodity Based Trust Shares"},
    {"CF", "Commodity Futures Trust Shares"},
    {"CL", "Commodity-Linked Securities"},
    {"CM", "Commodity Index Trust Shares"},
    {"CO", "Collateralized Mortgage Obligation"},
    {"CT", "Currency Trust Shares"},
    {"CU", "Commodity-Currency-Linked Securities"},
    {"CW", "Currency Warrants"},
    {"D", "Global Depositary Shares"},
    {"E", "ETF-Portfolio Depositary Receipt"},
    {"EG", "Equity Gold Shares"},
    {"EI", "ETN-Equity Index-Linked Securities"},
    {"EM", "NextShares Exchange Traded Managed Fund*"},
    {"EN", "Exchange Traded Notes"},
    {"EU", "Equity Units"},
    {"F", "HOLDRS"},
    {"FI", "ETN-Fixed Income-Linked Securities"},
    {"FL", "ETN-Futures-Linked Securities"},
    {"G", "Global Shares"},
    {"I", "ETF-Index Fund Shares"},
    {"IR", "Interest Rate"},
    {"IW", "Index Warrant"},
    {"IX", "Index-Linked Exchangeable Notes"},
    {"J", "Corporate Backed Trust Security"},
    {"L", "Contingent Litigation Right"},
    {"LL", "Limited Liability Company (LLC)"},
    {"M", "Equity-Based Derivative"},
    {"MF", "Managed Fund Shares"},
    {"ML", "ETN-Multi-Factor Index-Linked Securities"},
    {"MT", "Managed Trust Securities"},
    {"N", "NY Registry Shares"},
    {"O", "Open Ended Mutual Fund"},
    {"P", "Privately Held Security"},
    {"PP", "Poison Pill"},
    {"PU", "Partnership Units"},
    {"Q", "Closed-End Funds"},
    {"R", "Reg-S"},
    {"RC", "Commodity-Redeemable Commodity-Linked Securities"},
    {"RF", "ETN-Redeemable Futures-Linked Securities"},
    {"RT", "REIT"},
    {"RU", "Commodity-Redeemable Currency-Linked Securities"},
    {"S", "SEED"},
    {"SC", "Spot Rate Closing"},
    {"SI", "Spot Rate Intraday"},
    {"T", "Tracking Stock"},
    {"TC", "Trust Certificates"},
    {"TU", "Trust Units"},
    {"U", "Portal"},
    {"V", "Contingent Value Right"},
    {"W", "Trust Issued Receipts"},
    {"WC", "World Currency Option"},
    {"X", "Trust"},
    {"Y", "Other"},
    {"Z", "Not Applicable"},

};

const std::map<std::string, std::string> TRADING_ACTION_REASON_CODES = {
    {"T1", "Halt News Pending"},
    {"T2", "Halt News Disseminated"},
    {"T3", "News and Resumption Times"},
    {"T5", "Single Security Trading Pause In Effect"},
    {"T6", "Regulatory Halt - Extraordinary Market Activity"},
    {"T7", "Single Security Trading Pause / Quotation Only Period"},
    {"T8", "Halt ETF"},
    {"T12", "Trading Halted; For Information Requested by Listing Market"},
    {"H4", "Halt Non-Compliance"},
    {"H9", "Halt Filings Not Current"},
    {"H10", "Halt SEC Trading Suspension"},
    {"H11", "Halt Regulatory Concern"},
    {"O1", "Operations Halt; Contact Market Operations"},
    {"LUDP", "Volatility Trading Pause"},
    {"LUDS", "Volatility Trading Pause - Straddle Condition"},
    {"MWC0", "Market Wide Circuit Breaker Halt - Carry over from previous day"},
    {"MWC1", "Market Wide Circuit Breaker Halt - Level 1"},
    {"MWC2", "Market Wide Circuit Breaker Halt - Level 2"},
    {"MWC3", "Market Wide Circuit Breaker Halt - Level 3"},
    {"MWCQ", "Market Wide Circuit Breaker Resumption"},
    {"IPO1", "IPO Issue Not Yet Trading"},
    {"IPOQ", "IPO Security Released for Quotation (Nasdaq Securities Only)"},
    {"IPOE",
     "IPO Security — Positioning Window Extension (Nasdaq Securities Only)"},
    {"M1", "Corporate Action"},
    {"M2", "Quotation Not Available"},
    {"R1", "New Issue Available"},
    {"R2", "Issue Available"},
    {"R4",
     "Qualifications Issues Reviewed/Resolved; Quotations/Trading to Resume"},
    {"R9",
     "Filing Requirements Satisfied/Resolved; Quotations/Trading To Resume"},
    {"C3", "Issuer News Not Forthcoming; Quotations/Trading To Resume"},
    {"C4", "Qualifications Halt Ended; Maintenance Requirements Met; Resume"},
    {"C9",
     "Qualifications Halt Concluded; Filings Met; Quotes/Trades To Resume"},
    {"C11",
     "Trade Halt Concluded By Other Regulatory Authority; Quotes/Trades "
     "Resume"},
    {" ", "Reason Not Available"},
};

const std::map<char, std::string>  TRADING_STATES = {
    {'H', "Halted across all U.S. equity markets / SROs"},
    {'P', "Paused across all U.S. equity markets / SROs"},
    {'Q', "Quotation only period for cross-SRO halt or pause"},
    {'T', "Trading on NASDAQ"},
};

const std::map<char, std::string>  MARKET_MAKER_MODE = {
    {'N', "Normal"},
    {'P', "Passive"},
    {'S', "Syndicate"},
    {'R', "Pre-syndicate"},
    {'L', "Penalty"},
};

const std::map<char, std::string>  MARKET_PARTICIPANT_STATE = {
    {'A', "Active"},
    {'E', "Excused"},
    {'W', "Withdrawn"},
    {'S', "Suspended"},
    {'D', "Deleted"},
};

const std::map<char, std::string>  PRICE_VARIATION_INDICATOR = {
    {'L', "Less than 1%"},
    {'1', "1 to 1.99%"},
    {'2', "2 to 2.99%"},
    {'3', "3 to 3.99%"},
    {'4', "4 to 4.99%"},
    {'5', "5 to 5.99%"},
    {'6', "6 to 6.99%"},
    {'7', "7 to 7.99%"},
    {'8', "8 to 8.99%"},
    {'9', "9 to 9.99%"},
    {'A', "10 to 19.99%"},
    {'B', "20 to 29.99%"},
    {'C', "30% or greater"},
    {' ', "Cannot be calculated"},
};

}  // namespace indicators

}  // namespace itch
