import json
from Deribit_util.DataType import *
from Deribit_util.Util import *

class Deribit_Impl(object):
    def __init__(self, client_id, client_secret, server_url="wss://test.deribit.com/ws/api/v2"):
        self._client_id = client_id
        self._client_secret = client_secret
        self._server_url = server_url

    def get_auth_req(self):
        msg = \
            {
                "jsonrpc": "2.0",
                "id": 9929,
                "method": "public/auth",
                "params": {
                    "grant_type": "client_credentials",
                    "client_id": self._client_id,
                    "client_secret": self._client_secret
                }
            }
        return json.dumps(msg)

    def get_positions_req(self,currency,assetType):
        if(currency!= Currency.BTC and currency!= Currency.BTC and currency!= Currency.BTC):
            print('Error:,Currency should be BTC/ETH/USDT')
            return ''
        kind = 'option'
        if(assetType== AssetType.FUTURES):
            kind = 'future'
        msg = \
            {
                "jsonrpc": "2.0",
                "id": 2236,
                "method": "private/get_positions",
                "params": {
                    "currency": currency.value,
                    "kind": kind
                }
            }
        return json.dumps(msg)

    def get_insertOrder_req(self, td):
        #td = TDInputField()
        code = td.InstrumentID
        vol = td.VolumeTotal
        pt = 'market'
        if(td.PriceType==PriceType.LIMIT):
            pt = 'limit'

        if(td.Direct==Direction.LONG):
            if(pt=='market'):
                msg = \
                    {
                        "jsonrpc": "2.0",
                        "id": 5275,
                        "method": "private/buy",
                        "params": {
                            "instrument_name": code,
                            "amount": vol,
                            "type": "market",
                            "label": td.LocalOrderID
                        }
                    }
                return json.dumps(msg)
            else:
                msg = \
                    {
                        "jsonrpc": "2.0",
                        "id": 5275,
                        "method": "private/buy",
                        "params": {
                            "instrument_name": code,
                            "amount": vol,
                            "type": "limit",
                            "label": td.LocalOrderID,
                            "price": str(td.LimitPrice)
                        }
                    }
                return json.dumps(msg)
        else:
            if (pt == 'market'):
                msg = \
                    {
                        "jsonrpc": "2.0",
                        "id": 2148,
                        "method": "private/sell",
                        "params": {
                            "instrument_name": code,
                            "amount": vol,
                            "type": "market",
                            "label": td.LocalOrderID
                        }
                    }
                return json.dumps(msg)
            else:
                msg = \
                    {
                        "jsonrpc": "2.0",
                        "id": 2148,
                        "method": "private/sell",
                        "params": {
                            "instrument_name": code,
                            "amount": vol,
                            "type": "limit",
                            "label": td.LocalOrderID,
                            "price": str(td.LimitPrice)
                        }
                    }
                return json.dumps(msg)

    def get_cancel_req(self, sysid):
        msg = \
            {
                "jsonrpc": "2.0",
                "id": 4214,
                "method": "private/cancel",
                "params": {
                    "order_id": sysid
                }
            }
        return json.dumps(msg)

    def get_cancelAll_req(self):
        msg = \
            {
                "jsonrpc": "2.0",
                "id": 8748,
                "method": "private/cancel_all",
                "params": {

                }
            }
        return json.dumps(msg)

if __name__ == '__main__':
    imp = Deribit_Impl('','')
    json = imp.get_positions_req(Currency.BTC,AssetType.FUTURES)
    print(json)

