import Deribit_util.Deribit_API
import time
import Deribit_util.Util
from Deribit_util.DataType import *
from Deribit_util.utils.timeservice import *

dapi = None


#usdt本位合约的订单事件回调函数，包括成交和下单事件回调,见https://binance-docs.github.io/apidocs/futures/cn/#060a012f0b
#只有状态发生变化，比如成交，撤单，部分成交时会推送结果，下同
def ub_order_ws_callback(rsp):
    print('rcv ub order event')
    #数据结构见ord = Binance_util.Binance_ub_model.orderupdate.OrderUpdate()
    ord = rsp
    print(('InstruID:%s,clientID:%s,price:%f,SysID:%d,OrderStatus:%s,ordQty:%f')
          %(ord.symbol,ord.clientOrderId,ord.price,ord.orderId,ord.orderStatus,ord.origQty))

#币本位合约的订单事件回调函数，包括成交和下单事件回调,见https://binance-docs.github.io/apidocs/delivery/cn/#ef516897ee
def cb_order_ws_callback(rsp):
    print('rcv cb order event')
    #数据结构见ord = Binance_util.Binance_cb_model.orderupdate.OrderUpdate()
    ord = rsp
    print(('InstruID:%s,clientID:%s,price:%f,SYSID:%d,OrderStatus:%s,ordQty:%d')
          % (ord.symbol,ord.clientOrderId, ord.price, ord.orderId, ord.orderStatus, ord.origQty))

def on_ub_md_ticker(rsp):
    print('rcv ub md ticker')
    md = rsp
    print(('InstruID:%s,price:%f,Cur_price:%f,fundingRate:%s,nextFundingTime:%d')
          % (md.symbol,md.markPrice,md.IndexPrice,md.fundingRate,md.nextFundingTime))

def on_cb_md_ticker(rsp):
    print('rcv cb md ticker')
    md = rsp
    print(('InstruID:%s,price:%f')
          % (md.symbol,md.markPrice))


def test_ticker():
    # 参数留空就查询全部合约,下同
    print('查询全部usdt合约的最新价，如果您需要持续获取最新价，建议通过websocket订阅和推送')
    print('-------------------------------------')
    sendOK,errmsg,ub_all_tick = bapi.Get_Instru_Tick(Product.USDT_FUT, None)
    # Binance_util.Binance_ub_model.symbolprice.SymbolPrice，对应https://binance-docs.github.io/apidocs/futures/cn/#8ff46b58de
    for it in ub_all_tick:
        print(it.symbol, it.price, it.time)
    print('查询全部币合约的最新价，如果您需要持续获取最新价，建议通过websocket订阅和推送')
    print('-------------------------------------')
    sendOK,errmsg,cb_all_tick = bapi.Get_Instru_Tick(Product.USD_FUT, None)
    # Binance_util.Binance_cb_model.symbolprice.SymbolPrice,对应https://binance-docs.github.io/apidocs/delivery/cn/#8ff46b58de
    for it in cb_all_tick:
        print(it.symbol, it.price, it.time, it.ps)

def test_order():

    inp = Deribit_util.Util.TDInputField()
    inp.Direct = Direction.LONG
    inp.InstrumentID = 'BTCUSDT'
    inp.PriceType = PriceType.LIMIT
    inp.ProductType = Product.USDT_FUT
    inp.LocalOrderID = str(get_current_timestamp()) + 'PioNex_Ub'
    inp.LimitPrice = 51000.0
    inp.VolumeTotal = 0.01

    # rsp数据结构见Binance_util.Binance_ub_model.order.Order和https://binance-docs.github.io/apidocs/futures/cn/#trade-3
    # U本位期货合约下单测试
    # 下单成功返回True,' ',Order,失败返回False，errMsg，None
    # GetOrder，CancelOrder同上
    sendOK,errMsg,rsp = bapi.InputOrder(inp)
    print(sendOK,errMsg,rsp)
    time.sleep(1)

    # 查询order，数据结构同上
    sendOK,errMsg,rsp  = bapi.GetOrder(Product.USDT_FUT, 'BTCUSDT', rsp.orderId, inp.LocalOrderID)
    print(sendOK, errMsg, rsp)
    # 撤单测试，sysID(rsp.orderID）和本地ID（inp.LocalOrderID）只需一个就可以撤单，下同
    # 也可以通过CancelAllOrder撤销全部订单
    sendOK,errMsg,rsp = bapi.CancelOrder(Product.USDT_FUT, 'BTCUSDT', rsp.orderId, inp.LocalOrderID)
    print(sendOK, errMsg, rsp)
    return True
    inp2 = Binance_util.Util.TDInputField()
    inp2.Direct = Direction.LONG
    inp2.InstrumentID = 'BTCUSD_PERP'
    inp2.PriceType = PriceType.LIMIT
    inp2.ProductType = Product.USD_FUT
    inp2.LocalOrderID = str(get_current_timestamp()) + 'PioNex_Cb'
    inp2.LimitPrice = 50000.0
    # inp.OffsetFlag = Offset.OPEN 非必须，似乎自动算开平
    inp2.VolumeTotal = 1

    # rsp数据结构见Binance_util.Binance_cb_model.order.Order和https://binance-docs.github.io/apidocs/delivery/cn/#trade-2
    # 币本位期货合约下单测试
    sendOK, errMsg, rsp2 = bapi.InputOrder(inp2)
    print(sendOK, errMsg, rsp2.orderId)
    time.sleep(1)
    # 查询单子，数据结构同上
    sendOK, errMsg, rsp2 = bapi.GetOrder(Product.USD_FUT, 'BTCUSD_PERP', rsp2.orderId, inp2.LocalOrderID)
    print(sendOK, errMsg, rsp2)
    # 撤单测试，sysID(rsp.orderID）和本地ID（inp.LocalOrderID）只需一个就可以撤单，下同
    sendOK, errMsg, rsp2 = bapi.CancelOrder(Product.USD_FUT, 'BTCUSD_PERP', rsp2.orderId, inp2.LocalOrderID)
    print(sendOK, errMsg, rsp2)


def test_pos_accout():
    #将usdt账户的持仓模式改成双向，同时显示多仓和空仓
    #bapi.SetPositionMode(Product.USDT_FUT,True)
    # 将币本位账户的持仓模式改成单，只显示净头寸
    #bapi.SetPositionMode(Product.USD_FUT, False)

    #查询usdt本位合约的仓位
    sendOK, errMsg, ub_pos = bapi.GetPositon(Product.USDT_FUT)
    #model:Binance_util.Binance_ub_model.position.Position,doc:https://binance-docs.github.io/apidocs/futures/cn/#trade-13
    for it in ub_pos:
        #合约名，持仓模式，仓位，负数为空头
        if(it.positionAmt!=0.0):
            print(it.symbol,it.positionSide,it.positionAmt)

    # model:Binance_util.Binance_cb_model.position.Position,doc:https://binance-docs.github.io/apidocs/delivery/cn/#trade-14
    sendOK, errMsg, cb_pos = bapi.GetPositon(Product.USD_FUT)
    for it in cb_pos:
        #合约名，持仓模式，仓位，负数为空头
        if (it.positionAmt != 0.0):
            print(it.symbol,it.positionSide,it.positionAmt)

    #查询账户资产
    #Binance_util.Binance_ub_model.balancev2.BalanceV2.AccountInformationV2，Doc:https://binance-docs.github.io/apidocs/futures/cn/#v2-user_data
    sendOK, errMsg, ub_acc = bapi.GetAccountBalance(Product.USDT_FUT)
    for it in ub_acc:
        if(it.balance>0):
            print(it.asset,it.balance,it.availableBalance,it.crossUnPnl)
    #Binance_util.Binance_cb_model.balance.Balance,Doc:https://binance-docs.github.io/apidocs/delivery/cn/#user_data-7
    sendOK, errMsg, cb_acc = bapi.GetAccountBalance(Product.USD_FUT)
    for it in cb_acc:
        if(it.balance>0):
            print(it.asset, it.balance, it.availableBalance, it.crossUnPnl)
    pass

#订阅行情测试，币安要求订阅行情时必须要用小写字母，API做了大小写自动转换
def test_sub_md():
    #bapi.sub_md_tick(Product.USD_FUT,'BTCUSD_220325')
    bapi.sub_md_tick(Product.USDT_FUT, 'BTCUSDT')
    pass

def test_marker_order():
    inp2 = Deribit_util.Util.TDInputField()
    inp2.Direct = Direction.LONG
    inp2.InstrumentID = 'BTCUSD_PERP'
    inp2.PriceType = PriceType.MARKET
    inp2.ProductType = Product.USD_FUT
    inp2.LocalOrderID = str(get_current_timestamp()) + 'PioNex_Cb'
    inp2.LimitPrice = 0#市价单价格随意填，因为不会发给服务端
    # inp.OffsetFlag = Offset.OPEN 非必须，似乎自动算开平
    inp2.VolumeTotal = 1

    # rsp数据结构见Binance_util.Binance_cb_model.order.Order和https://binance-docs.github.io/apidocs/delivery/cn/#trade-2
    # 币本位期货合约下单测试
    sendOK, errMsg, rsp2 = bapi.InputOrder(inp2)
    pass

def get_instru_info():
    print('获取币本位合约信息，在浏览器里打开https://dapi.binance.com/dapi/v1/exchangeInfo，文档https://binance-docs.github.io/apidocs/delivery/cn/#0f3f2d5ee7')
    print('获取u本位合约信息， 在浏览器里打开https://fapi.binance.com/fapi/v1/exchangeInfo，文档https://binance-docs.github.io/apidocs/futures/cn/#0f3f2d5ee7')


if __name__ == '__main__':
    bapi = Deribit_util.Deribit_API.Binance_API(api_key='SbKHYMQZdWdUwepHRaejLdZFYy4C5v8h1P98V4cE9M19gSl1LiOSYy70SkeiGObp',
                                                secret_key='k8OIfZ4wmAqobAuCHgRWhBBD0AILzF1YTuyygUuBvduWLukvm1N7uLYpfo3vKRIH')
    bapi.on_ub_order_callback = ub_order_ws_callback
    bapi.on_cb_order_callback = cb_order_ws_callback
    bapi.on_ub_md_callback = on_ub_md_ticker
    bapi.on_cb_md_callback = on_cb_md_ticker
    #连接ws服务
    bapi.init()
    while True:
        ws_ub_ready = True
        ws_cb_ready = True
        time.sleep(1)
        if(ws_cb_ready and ws_ub_ready):
            print('ub and cb ws_connection ready')
            break

        if (bapi.ub_connection.state != Deribit_util.WS_ub_Connection.ConnectionState.CONNECTED):
            print('wait ub ws_connection ready')
            ws_ub_ready = False
        if (bapi.ub_connection.state != Deribit_util.WS_ub_Connection.ConnectionState.CONNECTED):
            print('wait ub ws_connection ready')
            ws_cb_ready = False

    #测试获取全部行情，如果需要持续获取最新行情，请务必通过websocket订阅
    #test_ticker()
    #测试查询持仓和账户相关
    test_pos_accout()
    #测试报撤单
    #test_order()
    #测试websocket行情推送，目前一个行情就会启动一个ws线程，建议不要订阅太多。。。。。。
    #test_sub_md()
    #测试市价单
    #test_marker_order()
    #获取合约信息
    #get_instru_info()
    while True:
        time.sleep(10)