"""Indicator/code lookup tables for ITCH 5.0 fields.

Vendored verbatim from ``itch.indicators`` in the pure-Python ``itchfeed`` package
so that code reading these maps works unchanged against the native backend. Each
table maps a single- or multi-byte code to its human-readable description.
"""

SYSTEM_EVENT_CODES = {
    b"O": "Start of Messages",
    b"S": "Start of System hours",
    b"Q": "Start of Market hours",
    b"M": "End of Market hours",
    b"E": "End of System hours",
    b"C": "End of Messages",
}


MARKET_CATEGORY = {
    b"N": "NYSE",
    b"A": "AMEX",
    b"P": "Arca",
    b"Q": "NASDAQ Global Select",
    b"G": "NASDAQ Global Market",
    b"S": "NASDAQ Capital Market",
    b"Z": "BATS",
    b"V": "Investors Exchange",
    b" ": "Not available",
}


FINANCIAL_STATUS_INDICATOR = {
    b"D": "Deficient",
    b"E": "Delinquent",
    b"Q": "Bankrupt",
    b"S": "Suspended",
    b"G": "Deficient and Bankrupt",
    b"H": "Deficient and Delinquent",
    b"J": "Delinquent and Bankrupt",
    b"K": "Deficient, Delinquent and Bankrupt",
    b"C": "Creations and/or Redemptions Suspended for Exchange Traded Product",
    b"N": "Normal (Default): Issuer is NOT Deficient, Delinquent, or Bankrupt",
    b" ": "Not available. Firms should refer to SIAC feeds for code if needed",
}


ISSUE_CLASSIFICATION_VALUES = {
    b"A": "American Depositary Share",
    b"B": "Bond",
    b"C": "Common Stock",
    b"F": "Depository Receipt",
    b"I": "144A",
    b"L": "Limited Partnership",
    b"N": "Notes",
    b"O": "Ordinary Share",
    b"P": "Preferred Stock",
    b"Q": "Other Securities",
    b"R": "Right",
    b"S": "Shares of Beneficial Interest",
    b"T": "Convertible Debenture",
    b"U": "Unit",
    b"V": "Units/Beneficial Interest",
    b"W": "Warrant",
}


ISSUE_SUB_TYPE_VALUES = {
    b"A": "Preferred Trust Securities",
    b"AI": "Alpha Index ETNs",
    b"B": "Index Based Derivative",
    b"C": "Common Shares",
    b"CB": "Commodity Based Trust Shares",
    b"CF": "Commodity Futures Trust Shares",
    b"CL": "Commodity-Linked Securities",
    b"CM": "Commodity Index Trust Shares",
    b"CO": "Collateralized Mortgage Obligation",
    b"CT": "Currency Trust Shares",
    b"CU": "Commodity-Currency-Linked Securities",
    b"CW": "Currency Warrants",
    b"D": "Global Depositary Shares",
    b"E": "ETF-Portfolio Depositary Receipt",
    b"EG": "Equity Gold Shares",
    b"EI": "ETN-Equity Index-Linked Securities",
    b"EM": "NextShares Exchange Traded Managed Fund*",
    b"EN": "Exchange Traded Notes",
    b"EU": "Equity Units",
    b"F": "HOLDRS",
    b"FI": "ETN-Fixed Income-Linked Securities",
    b"FL": "ETN-Futures-Linked Securities",
    b"G": "Global Shares",
    b"I": "ETF-Index Fund Shares",
    b"IR": "Interest Rate",
    b"IW": "Index Warrant",
    b"IX": "Index-Linked Exchangeable Notes",
    b"J": "Corporate Backed Trust Security",
    b"L": "Contingent Litigation Right",
    b"LL": "Limited Liability Company (LLC)",
    b"M": "Equity-Based Derivative",
    b"MF": "Managed Fund Shares",
    b"ML": "ETN-Multi-Factor Index-Linked Securities",
    b"MT": "Managed Trust Securities",
    b"N": "NY Registry Shares",
    b"O": "Open Ended Mutual Fund",
    b"P": "Privately Held Security",
    b"PP": "Poison Pill",
    b"PU": "Partnership Units",
    b"Q": "Closed-End Funds",
    b"R": "Reg-S",
    b"RC": "Commodity-Redeemable Commodity-Linked Securities",
    b"RF": "ETN-Redeemable Futures-Linked Securities",
    b"RT": "REIT",
    b"RU": "Commodity-Redeemable Currency-Linked Securities",
    b"S": "SEED",
    b"SC": "Spot Rate Closing",
    b"SI": "Spot Rate Intraday",
    b"T": "Tracking Stock",
    b"TC": "Trust Certificates",
    b"TU": "Trust Units",
    b"U": "Portal",
    b"V": "Contingent Value Right",
    b"W": "Trust Issued Receipts",
    b"WC": "World Currency Option",
    b"X": "Trust",
    b"Y": "Other",
    b"Z": "Not Applicable",
}


TRADING_STATES = {
    b"H": "Halted across all U.S. equity markets / SROs",
    b"P": "Paused across all U.S. equity markets / SROs",
    b"Q": "Quotation only period for cross-SRO halt or pause",
    b"T": "Trading on NASDAQ",
}


TRADING_ACTION_REASON_CODES = {
    b"T1": "Halt News Pending",
    b"T2": "Halt News Disseminated",
    b"T3": "News and Resumption Times",
    b"T5": "Single Security Trading Pause In Effect",
    b"T6": "Regulatory Halt - Extraordinary Market Activity",
    b"T7": "Single Security Trading Pause / Quotation Only Period",
    b"T8": "Halt ETF",
    b"T12": "Trading Halted; For Information Requested by Listing Market",
    b"H4": "Halt Non-Compliance",
    b"H9": "Halt Filings Not Current",
    b"H10": "Halt SEC Trading Suspension",
    b"H11": "Halt Regulatory Concern",
    b"O1": "Operations Halt; Contact Market Operations",
    b"LUDP": "Volatility Trading Pause",
    b"LUDS": "Volatility Trading Pause - Straddle Condition",
    b"MWC0": "Market Wide Circuit Breaker Halt - Carry over from previous day",
    b"MWC1": "Market Wide Circuit Breaker Halt - Level 1",
    b"MWC2": "Market Wide Circuit Breaker Halt - Level 2",
    b"MWC3": "Market Wide Circuit Breaker Halt - Level 3",
    b"MWCQ": "Market Wide Circuit Breaker Resumption",
    b"IPO1": "IPO Issue Not Yet Trading",
    b"IPOQ": "IPO Security Released for Quotation (Nasdaq Securities Only)",
    b"IPOE": "IPO Security — Positioning Window Extension (Nasdaq Securities Only)",
    b"M1": "Corporate Action",
    b"M2": "Quotation Not Available",
    b"R1": "New Issue Available",
    b"R2": "Issue Available",
    b"R4": "Qualifications Issues Reviewed/Resolved; Quotations/Trading to Resume",
    b"R9": "Filing Requirements Satisfied/Resolved; Quotations/Trading To Resume",
    b"C3": "Issuer News Not Forthcoming; Quotations/Trading To Resume",
    b"C4": "Qualifications Halt Ended; Maintenance Requirements Met; Resume",
    b"C9": "Qualifications Halt Concluded; Filings Met; Quotes/Trades To Resume",
    b"C11": "Trade Halt Concluded By Other Regulatory Authority; Quotes/Trades Resume",
    b" ": "Reason Not Available",
}


PRIMARY_MARKET_MAKER = {
    b"Y": "Primary market maker",
    b"N": "Non-primary market maker",
}


MARKET_MAKER_MODE = {
    b"N": "Normal",
    b"P": "Passive",
    b"S": "Syndicate",
    b"R": "Pre-syndicate",
    b"L": "Penalty",
}


MARKET_PARTICIPANT_STATE = {
    b"A": "Active",
    b"E": "Excused",
    b"W": "Withdrawn",
    b"S": "Suspended",
    b"D": "Deleted",
}


PRICE_VARIATION_INDICATOR = {
    b"L": "Less than 1%",
    b"1": "1 to 1.99%",
    b"2": "2 to 2.99%",
    b"3": "3 to 3.99%",
    b"4": "4 to 4.99%",
    b"5": "5 to 5.99%",
    b"6": "6 to 6.99%",
    b"7": "7 to 7.99%",
    b"8": "8 to 8.99%",
    b"9": "9 to 9.99%",
    b"A": "10 to 19.99%",
    b"B": "20 to 29.99%",
    b"C": "30% or greater",
    b" ": "Cannot be calculated",
}
