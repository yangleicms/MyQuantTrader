import time
from Deribit_util.WS_Request import WebsocketRequest
from Deribit_util.utils.channels import *
from Deribit_util.utils.channelparser import ChannelParser
from Deribit_util.utils.timeservice import *
from Deribit_util.utils.inputchecker import *


class WebsocketRequestImpl(object):

    def __init__(self):
        pass

    def subscribe_user_data_event(self, callback, error_handler=None):
        request = WebsocketRequest()
        request.subscription_handler = None
        request.json_parser = None
        request.update_callback = callback
        request.error_handler = error_handler

        return request



