from enum import *

class Direction(Enum):
    """
    Direction of order/trade/position.
    """
    LONG = 0
    SHORT = 1


class Offset(Enum):
    """
    Offset of order/trade.
    """
    OPEN = 0
    CLOSE = 1
    CLOSETODAY = 2
    CLOSEYESTERDAY = 3

class PriceType(Enum):
    LIMIT = 0
    MARKET = 1

class TimeInForce():
    GTC =1
    FOK = 1
    IOC = 1

class OrderStatus(Enum):
    """
    Order status.
    """
    OrderStatus_AllTraded = 0
    OrderStatus_PartTradedQueueing = 1
    OrderStatus_PartTradedNotQueueing = 2
    OrderStatus_NoTradeQueueing = 3
    OrderStatus_NoTradeNotQueueing = 4
    OrderStatus_Canceled = 5

    OrderStatus_UnKnown = 6
    OrderStatus_Error = 7
    OrderStatus_Canceling = 8
    OrderStatus_Sync = 9
    OrderStatus_PartialTrading_PartialCanceling = 10
    OrderStatus_PartialTraded_PartialCanceled = 11


class AssetType(Enum):
    """
    Product class.
    """
    STOCK = 1
    FUTURES = 2
    OPTION_CALL_European = 3
    OPTION_PUT_European = 4
    OPTION_CALL_American = 5
    OPTION_PUT_American = 6
    INDEX = 7
    FOREX = 8
    SPOT = 9
    ETF = 10
    BOND = 11
    WARRANT = 12
    SPREAD = 13
    FUND = 14
    Option = 15



class OrderType(Enum):
    """
    Order type.
    """
    LIMIT = "1"
    MARKET = "2"
    STOP = "3"
    FAK = "4"
    FOK = "5"


class OptionType(Enum):
    """
    Option type.
    """
    CALL = "看涨期权"
    PUT = "看跌期权"


class Exchange(Enum):
    """
    Exchange.
    """
    # Chinese
    CFFEX = "CFFEX"         # China Financial Futures Exchange
    SHFE = "SHFE"           # Shanghai Futures Exchange
    CZCE = "CZCE"           # Zhengzhou Commodity Exchange
    DCE = "DCE"             # Dalian Commodity Exchange
    INE = "INE"             # Shanghai International Energy Exchange
    SSE = "SSE"             # Shanghai Stock Exchange
    SZSE = "SZSE"           # Shenzhen Stock Exchange
    SGE = "SGE"             # Shanghai Gold Exchange
    WXE = "WXE"             # Wuxi Steel Exchange
    CFETS = "CFETS"         # China Foreign Exchange Trade System

    # Global
    SMART = "SMART"         # Smart Router for US stocks
    NYSE = "NYSE"           # New York Stock Exchnage
    NASDAQ = "NASDAQ"       # Nasdaq Exchange
    ARCA = "ARCA"           # ARCA Exchange
    EDGEA = "EDGEA"         # Direct Edge Exchange
    ISLAND = "ISLAND"       # Nasdaq Island ECN
    BATS = "BATS"           # Bats Global Markets
    IEX = "IEX"             # The Investors Exchange
    NYMEX = "NYMEX"         # New York Mercantile Exchange
    COMEX = "COMEX"         # COMEX of CME
    GLOBEX = "GLOBEX"       # Globex of CME
    IDEALPRO = "IDEALPRO"   # Forex ECN of Interactive Brokers
    CME = "CME"             # Chicago Mercantile Exchange
    ICE = "ICE"             # Intercontinental Exchange
    SEHK = "SEHK"           # Stock Exchange of Hong Kong
    HKFE = "HKFE"           # Hong Kong Futures Exchange
    HKSE = "HKSE"           # Hong Kong Stock Exchange
    SGX = "SGX"             # Singapore Global Exchange
    CBOT = "CBT"            # Chicago Board of Trade
    CBOE = "CBOE"           # Chicago Board Options Exchange
    CFE = "CFE"             # CBOE Futures Exchange
    DME = "DME"             # Dubai Mercantile Exchange
    EUREX = "EUX"           # Eurex Exchange
    APEX = "APEX"           # Asia Pacific Exchange
    LME = "LME"             # London Metal Exchange
    BMD = "BMD"             # Bursa Malaysia Derivatives
    TOCOM = "TOCOM"         # Tokyo Commodity Exchange
    EUNX = "EUNX"           # Euronext Exchange
    KRX = "KRX"             # Korean Exchange
    OTC = "OTC"             # OTC Product (Forex/CFD/Pink Sheet Equity)
    IBKRATS = "IBKRATS"     # Paper Trading Exchange of IB

    # CryptoCurrency
    BITMEX = "BITMEX"
    OKEX = "OKEX"
    HUOBI = "HUOBI"
    BITFINEX = "BITFINEX"
    BINANCE = "BINANCE"
    BYBIT = "BYBIT"         # bybit.com
    COINBASE = "COINBASE"
    DERIBIT = "DERIBIT"
    GATEIO = "GATEIO"
    BITSTAMP = "BITSTAMP"

    # Special Function
    LOCAL = "LOCAL"         # For local generated data


class Currency(Enum):
    """
    Currency.
    """
    USD = "USD"
    HKD = "HKD"
    CNY = "CNY"
    BTC = "BTC"
    ETH = "ETH"
    USDT = "USDT"


class Interval(Enum):
    """
    Interval of bar data.
    """
    MINUTE = "1m"
    HOUR = "1h"
    DAILY = "d"
    WEEKLY = "w"
    TICK = "tick"

if __name__ == '__main__':
    print((Currency.USDT.value))