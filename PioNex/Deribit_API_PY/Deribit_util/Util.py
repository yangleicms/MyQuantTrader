from Deribit_util.DataType import *

#下单和撤单用的数据结构
class TDInputField():
    LocalOrderID = ''
    LimitPrice = 0.0 #orderprice
    StrategyID=''
    InvestorID=''
    InstrumentID=''
    VolumeTotal=0.0
    Direct = Direction.LONG
    OffsetFlag = Offset.OPEN
    PriceType = PriceType.LIMIT
    ExchangeIdx = Exchange.BINANCE
    OptType = OptionType.CALL#只有期权合约 需要
    CurType = Currency.USDT#
    ProductType =Product.USDT_FUT#默认下单类型为币本位期货

class TDEventType(Enum):
    TDRtnOrder = 1#订单状态变化
    TDRtnTrade = 2#成交事件
    TDRspOrderInsert =3#下单出错
    TDRspOrderAction =4#撤单出错

class TDEvent():
    eventType = TDEventType.TDRtnOrder
    ErrorID = 0
    ErrorMsg =''
    orderID = 0
    OrderSysID = ''
    VolumeTotal = 0
    TradeVolume =0
    TradeID = ''
    TradePrice = 0.0
    OrdStatus = OrderStatus.OrderStatus_AllTraded
    UpTime = 0


