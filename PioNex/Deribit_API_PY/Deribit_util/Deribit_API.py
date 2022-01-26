import Deribit_util.urlRequest
import Deribit_util.WS_ReqImpl
import Deribit_util.WS_Connection
from Deribit_util.WS_Watch_Dog import *
from Deribit_util.utils import *
import Deribit_util.Deribit_Impl
from Deribit_util.DataType import *
from Deribit_util.Util import *
import time

client_id = "ByTHzEjk"
client_secret = "gbBrGeyHV9Z7dBb2vS0A5M-Dzi7zhvQsjJMOVaIsDEA"

class Deribit_API(object):
    def __init__(self, client_id, client_secret):
        self._client_id = client_id
        self._client_secret = client_secret
        self.impl = Deribit_util.Deribit_Impl.Deribit_Impl(client_id,client_secret)
        self.keep_thread = None

        self.ws_connection = None
        self.ws_impl = Deribit_util.WS_ReqImpl.WebsocketRequestImpl()
        self.other_connection = dict()

        self.refresh_token = ''
        self.access_token = ''
        self.is_auth = False

        self.on_order_callback = None
        self.on_trade_callback = None
        self.on_pos_callback = None

        self.on_md_callback = None
        self.func_map = {}

    def error_process(self, exception):
        print(exception)

    def on_event_dispatch(self, jsonmsg):
        id = jsonmsg.get_int("id")
        dict = jsonmsg.convert_2_dict()
        if(self.func_map.__contains__(id)):
            func = self.func_map[id](dict)

    def on_rtn_auth(self,dict):
        self.refresh_token = dict["result"]["refresh_token"]
        self.access_token = dict["result"]["access_token"]
        self.is_auth = True
        print('on_rtn_auth:'+str(self.refresh_token)+','+str(self.access_token))

    def on_rtn_pos(self, dict):
        res = dict["result"]
        if(self.on_pos_callback!=None):
            for it in res:
                self.on_pos_callback(it)

    def on_rtn_order(self,dict):
        ord = dict["result"]["order"]
        trd = dict["result"]["trades"]
        if (self.on_order_callback != None):
            self.on_order_callback(ord)
        if(self.on_trade_callback!=None):
            for it in trd:
                self.on_trade_callback(it)

    def init_func_map(self):
        self.func_map[9929] = self.on_rtn_auth
        self.func_map[2236] = self.on_rtn_pos
        self.func_map[5275] = self.on_rtn_order
        self.func_map[2148] = self.on_rtn_order
        self.func_map[4214] = self.on_rtn_order

    def init(self):
        self.init_func_map()
        url = "wss://www.deribit.com/ws/api/v2"
        cb_watch_dog = WebSocketWatchDog(is_auto_connect=False)
        ws_req = self.ws_impl.subscribe_user_data_event(callback=self.on_event_dispatch,error_handler=self.error_process)
        self.ws_connection = Deribit_util.WS_Connection.WebsocketConnection(url,cb_watch_dog,ws_req)
        self.ws_connection.connect()

        try_num = 30
        while(True):
            try_num -= 1
            if(try_num<=0):
                print('Error:ws_connect time out!')
                return False
            time.sleep(0.1)
            if(self.ws_connection.state==ConnectionState.CONNECTED):
                print('ws connect Fin')
                break

        self.req_auth()

        try_num = 30
        while (True):
            try_num -= 1
            if (try_num <= 0):
                print('Error:Deribit auth time out!')
                return False
            time.sleep(0.1)
            if (self.is_auth == True):
                print('Deribit auth Fin')
                break

        return True

    def req_auth(self):
        json = self.impl.get_auth_req()
        self.ws_connection.send(json)

    def sub_md_tick(self,pt,instruID):
        pass

    def InputOrder(self,td_input):
        json = self.impl.get_insertOrder_req(td_input)
        self.ws_connection.send(json)

    def CancelOrder(self,sysid):
        json = self.impl.get_cancel_req(sysid)
        self.ws_connection.send(json)

    def GetOrder(self):
        pass

    def CancelAllOrder(self):
        json = self.impl.get_cancelAll_req()
        self.ws_connection.send(json)

    #InstrumentID为空则是查询所有的合约
    def Get_Instru_Tick(self,pt,InstrumentID):
        pass
    #参数：DataType:Currency,DataType:AssetType
    def GetPositon(self,currency,assetType):
        json = self.impl.get_positions_req(currency,assetType)
        self.ws_connection.send(json)

    def GetAccountBalance(self,pt):
        pass

if __name__ == '__main__':
    bapi = Deribit_API(client_id=client_id,client_secret=client_secret)
    bapi.init()
    #bapi.GetPositon(Currency.BTC,AssetType.Option)

    #insert
    td = TDInputField()
    td.InstrumentID = 'BTC-PERPETUAL'
    td.VolumeTotal = 10
    td.LocalOrderID = 1024
    td.PriceType = PriceType.LIMIT
    td.LimitPrice = 37000
    td.Direct = Direction.LONG


    bapi.InputOrder(td)
    time.sleep(2)
    bapi.CancelAllOrder()
    while True:
        time.sleep(10)