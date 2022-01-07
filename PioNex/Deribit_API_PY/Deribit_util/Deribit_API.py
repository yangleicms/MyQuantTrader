import Deribit_util.urlRequest
import Deribit_util.WS_ReqImpl
import Deribit_util.WS_Connection
from Deribit_util.WS_Watch_Dog import *
from Deribit_util.utils import *
import Deribit_util.Util
import Deribit_util.Deribit_Impl
from Deribit_util.DataType import *
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
        self.on_md_callback = None
        self.func_map = {}

    def error_process(self, exception):
        print(exception)

    def on_event_dispatch(self, jsonmsg):
        id = jsonmsg.get_int("id")
        if(self.func_map.__contains__(id)):
            func = self.func_map[id](jsonmsg)

    def on_rtn_auth(self,jsonmsg):
        dict = jsonmsg.convert_2_dict()
        self.refresh_token = dict["result"]["refresh_token"]
        self.access_token = dict["result"]["access_token"]
        self.is_auth = True
        print(self.refresh_token,self.access_token)

    def init_func_map(self):
        self.func_map[9929] = self.on_rtn_auth

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
        pass

    def CancelOrder(self,pt,InstrumentID,sysID,LocalorderID):
        pass

    def GetOrder(self,pt,InstrumentID,sysID,LocalorderID):
        pass

    def CancelAllOrder(self,pt,InstrumentID):
      pass

    #InstrumentID为空则是查询所有的合约
    def Get_Instru_Tick(self,pt,InstrumentID):
        pass

    def GetPositon(self,pt):
        pass

    #切换持仓模式，true为双向，false为单向
    def SetPositionMode(self,pt,dualSidePosition):
        pass

    def GetAccountBalance(self,pt):
        pass

if __name__ == '__main__':
    bapi = Deribit_API(client_id=client_id,client_secret=client_secret)
    bapi.init()
    while True:
        time.sleep(10)