from Deribit_util.Util import *
class DeribitApiException(Exception):
    RUNTIME_ERROR = "RuntimeError"
    INPUT_ERROR = "InputError"
    KEY_MISSING = "KeyMissing"
    SYS_ERROR = "SysError"
    SUBSCRIPTION_ERROR = "SubscriptionError"
    ENV_ERROR = "EnvironmentError"
    EXEC_ERROR = "ExecuteError"

    def __init__(self, error_code, error_message):
        self.error_code = error_code
        self.error_message = error_message

class RestApiRequest(object):
    def __init__(self):
        self.method = ""
        self.url = ""
        self.host = ""
        self.post_body = ""
        self.header = dict()
        self.json_parser = None
        self.header.update({"client_SDK_Version": "binance_futures-1.0.1-py3.7"})

class SubscribeMessageType:
    RESPONSE = "response"
    PAYLOAD = "payload"