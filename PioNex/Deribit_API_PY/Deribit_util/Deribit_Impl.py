import json
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
