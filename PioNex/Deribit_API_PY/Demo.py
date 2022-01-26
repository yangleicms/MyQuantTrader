from Deribit_util.Deribit_API import *
import time
from Deribit_util.Util import *
from Deribit_util.DataType import *
from Deribit_util.utils.timeservice import *

dapi = None


#deribit合约的订单事件回调函数，包括撤单和下单事件回调,见
'''
https://docs.deribit.com/#private-buy
https://docs.deribit.com/#private-sell
https://docs.deribit.com/#private-cancel
的"Order" json字段
参数dict则是封装了order json的字典
'''
#只有状态发生变化，比如成交，撤单，部分成交时会推送结果，下同
def on_rtn_order_callback(dict):
    print('on_rtn_order_callback rcv order event')
    print(dict)

#deribit合约的成交事件回调函数，见
'''
https://docs.deribit.com/#private-buy
https://docs.deribit.com/#private-sell
的 "trades" json字段
参数dict是封装了trades json的字典
'''

def on_rtn_trade_callback(dict):
    print('on_rtn_trade_callback rcv Trade event')
    print(dict)


def test_order():
    #下单
    td = TDInputField()
    td.InstrumentID = 'BTC-PERPETUAL'
    td.VolumeTotal = 10#单位为美元
    td.LocalOrderID = 1024#orderID,唯一
    td.PriceType = PriceType.LIMIT#or PriceType.MARKET
    td.LimitPrice = 37000
    td.Direct = Direction.LONG

    bapi.InputOrder(td)
    bapi.CancelAllOrder()


if __name__ == '__main__':
    client_id = "ByTHzEjk"
    client_secret = "gbBrGeyHV9Z7dBb2vS0A5M-Dzi7zhvQsjJMOVaIsDEA"

    bapi = Deribit_API(client_id=client_id, client_secret=client_secret)
    bapi.on_order_callback = on_rtn_order_callback
    bapi.on_order_callback = on_rtn_order_callback
    bapi.init()
    test_order()

    while True:
        time.sleep(10)