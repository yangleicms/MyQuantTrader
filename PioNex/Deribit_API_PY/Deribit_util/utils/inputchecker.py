import re
import time
from Deribit_util.Deribit_Util import DeribitApiException

reg_ex = "[ _`~!@#$%^&*()+=|{}':;',\\[\\].<>/?~！@#￥%……&*（）——+|{}【】‘；：”“’。，、？]|\n|\t"


def check_symbol(symbol):
    if not isinstance(symbol, str):
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] symbol must be string")
    if re.match(reg_ex, symbol):
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] " + symbol + "  is invalid symbol")


def check_symbol_list(symbols):
    if not isinstance(symbols, list):
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] symbols in subscription is not a list")
    for symbol in symbols:
        check_symbol(symbol)


def check_currency(currency):
    if not isinstance(currency, str):
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] currency must be string")
    if re.match(reg_ex, currency) is not None:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] " + currency + "  is invalid currency")


def check_range(value, min_value, max_value, name):
    if value is None:
        return
    if min_value > value or value > max_value:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR,
                                "[Input] " + name + " is out of bound. " + str(value) + " is not in [" + str(
                                    min_value) + "," + str(max_value) + "]")


def check_should_not_none(value, name):
    if value is None:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] " + name + " should not be null")


def check_should_none(value, name):
    if value is not None:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] " + name + " should be null")


def check_list(list_value, min_value, max_value, name):
    if list_value is None:
        return
    if len(list_value) > max_value:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR,
                                "[Input] " + name + " is out of bound, the max size is " + str(max_value))
    if len(list_value) < min_value:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR,
                                "[Input] " + name + " should contain " + str(min_value) + " item(s) at least")


def greater_or_equal(value, base, name):
    if value is not None and value < base:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR,
                                "[Input] " + name + " should be greater than " + base)


def format_date(value, name):
    if value is None:
        return None
    if not isinstance(value, str):
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] " + name + " must be string")
    try:
        new_time = time.strptime(value, "%Y-%m-%d")
        return time.strftime("%Y-%m-%d", new_time)
    except:
        raise DeribitApiException(DeribitApiException.INPUT_ERROR, "[Input] " + name + " is not invalid date format")
