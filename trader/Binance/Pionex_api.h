#pragma once

#ifndef PIONEX_API_H
#define PIONEX_API_H
#include "base_websocket.h"
#include "common.h"
#include "cell_log.h"
#include "msg.h"

using namespace cell::common;

#define OLD_PIONEX_HOST "https://exchangedev.bthub.com/tapi"
#define PIONEX_HOST "https://api.pionex.com"
#define PIONEX_WS "ws.pionex.com"

class PionexAccount
{
public:
	std::string m_api_key;
	std::string m_secret_key;
	std::shared_ptr<WebSocketSSLClient> wscli;
public:
	PionexAccount(const std::string api_key, const std::string secret_key)
	{
		m_api_key = api_key;
		m_secret_key = secret_key;
	}
	~PionexAccount()
	{

	}

	bool keysAreSet() {
		return ((m_api_key.size() == 0) || (m_secret_key.size() == 0));
	}

	int send_lws_msg(const char* str, int len)
	{
		std::string strr = std::string(str, len);
		wscli->Send(strr);
		return 0;
	}

	int send_lws_msg(std::string& strr)
	{
		wscli->Send(strr);
		return 0;
	}

	bool parse_string2json(std::string& str_result, Json::Value& json_result)
	{
		Json::Reader reader;
		json_result.clear();
#ifdef DEBUG
		std::cout << str_result << std::endl;
#endif
		bool res = reader.parse(str_result, json_result);
		return res;
	}

	std::string get_pionex_private_url() {
		std::string tp = std::to_string(get_current_ms_epoch());
		std::string data = "/ws?key=" + m_api_key + "&timestamp=" + tp + "websocket_auth";
		std::string signature = hmac_sha256(m_secret_key.c_str(), data.c_str());
		std::string url = "/ws?key=" + m_api_key + "&timestamp=" + tp + "&signature=" + signature;
		return url;
	}

	void sub_order_ws(std::string instrument)
	{
		Json::Value jsObj;
		jsObj["op"] = "SUBSCRIBE";
		jsObj["topic"] = "ORDER";
		jsObj["symbol"] = instrument.data();
		std::string jsonstr = jsObj.toStyledString();
		wscli->Send(jsonstr);
	}

	void sub_fill_ws(std::string instrument)
	{
		Json::Value fillObj;
		fillObj["op"] = "SUBSCRIBE";
		fillObj["topic"] = "FILL";
		fillObj["symbol"] = instrument.data();
		std::string str = fillObj.toStyledString();
		wscli->Send(str);
	}

	bool send_order(const char* symbol, const char* side,
		const char* type, const char* clientOrderId, double size,
		double price, double amount, bool IOC, Json::Value& json_result)
	{
		int sid = 0;

		std::string tot_url(PIONEX_HOST);
		std::string url = "/api/v1/trade/order?";
		std::string action = "POST";

		std::string tp = std::to_string(get_current_ms_epoch());
		std::string pre_data = std::string("timestamp=") + tp;
		url += pre_data;

		Json::Value jsObj;
		jsObj["symbol"] = symbol;
		jsObj["side"] = side;
		jsObj["type"] = type;

		if (strcmp(type, "MARKET") != 0)
		{
			jsObj["size"] = _toString(size);
			std::string sValue = _toString(price);
			jsObj["price"] = sValue;
		}
		else
		{
			jsObj["amount"] = amount;
		}
		if (clientOrderId != nullptr)
		{
			jsObj["clientOrderId"] = clientOrderId;
		}
		if (IOC) {
			jsObj["IOC"] = true;
		}
		std::string post_data = jsObj.toStyledString();
		std::vector <std::string> extra_http_header;
		std::string header_chunk("PIONEX-KEY: ");
		header_chunk.append(m_api_key);

		std::string header_SIGNATURE("PIONEX-SIGNATURE: ");
		std::string tot_str = action + url + post_data;

		std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
		header_SIGNATURE.append(signature);

		extra_http_header.push_back(header_chunk);
		extra_http_header.push_back(header_SIGNATURE);

		std::string str_result;
		tot_url += url;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		if (str_result.size() > 0) {
			try {
				parse_string2json(str_result, json_result);
			}
			catch (std::exception& e) {
				SPDLOG_ERROR("PionexAccount::send_order:{}", e.what());
				return false;
			}
		}
		else {
			SPDLOG_ERROR("<PionexAccount::send_order> Failed to get anything.");
			return false;
		}
		return true;
	}

	bool cancel_order(const char* symbol, uint64_t orderId, Json::Value& json_result)
	{
		std::string tot_url(PIONEX_HOST);
		std::string url = "/api/v1/trade/order?";
		std::string action = "DELETE";

		std::string tp = std::to_string(get_current_ms_epoch());
		std::string pre_data = std::string("timestamp=") + tp;
		url += pre_data;

		Json::Value jsObj;
		jsObj["symbol"] = symbol;
		jsObj["orderId"] = orderId;
		std::string post_data = jsObj.toStyledString();

		std::vector <std::string> extra_http_header;
		std::string header_chunk("PIONEX-KEY: ");
		header_chunk.append(m_api_key);

		std::string header_SIGNATURE("PIONEX-SIGNATURE: ");
		std::string tot_str = action + url + post_data;
		std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
		header_SIGNATURE.append(signature);

		extra_http_header.push_back(header_chunk);
		extra_http_header.push_back(header_SIGNATURE);

		std::string str_result;
		tot_url += url;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		if (str_result.size() > 0) {

			try {
				parse_string2json(str_result, json_result);
			}
			catch (std::exception& e) {
				SPDLOG_ERROR("PionexAccount::cancel_order:{}", e.what());
				return false;
			}
		}
		else {
			SPDLOG_ERROR("<PionexAccount::cancel_order> Failed to get anything.");
			return false;
		}
		return true;
	}

	bool get_order(const char* symbol, uint64_t orderId, Json::Value& json_result)//orderID
	{
		std::string tot_url(PIONEX_HOST);
		std::string url = "/api/v1/trade/order?";
		std::string action = "GET";

		std::string tp = std::to_string(get_current_ms_epoch());
		std::string pre_data = std::string("orderId=") + std::to_string(orderId) + std::string("&symbol=") + (symbol)
			+std::string("&timestamp=") + tp;

		url += pre_data;

		std::vector <std::string> extra_http_header;
		std::string header_chunk("PIONEX-KEY: ");
		header_chunk.append(m_api_key);

		std::string header_SIGNATURE("PIONEX-SIGNATURE: ");
		std::string tot_str = action + url;
		std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
		header_SIGNATURE.append(signature);

		extra_http_header.push_back(header_chunk);
		extra_http_header.push_back(header_SIGNATURE);

		std::string str_result;
		std::string post_data = "";
		tot_url += url;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		if (str_result.size() > 0) {

			try {
				parse_string2json(str_result, json_result);
			}
			catch (std::exception& e) {
				SPDLOG_ERROR("PionexAccount::get_order:{}", e.what());
				return false;
			}
		}
		else {
			SPDLOG_ERROR("<PionexAccount::get_order> Failed to get anything.");
			return false;
		}

		return true;
	}

	bool get_order(const char* symbol, const char* localOrderId, Json::Value& json_result)//localID
	{
		std::string tot_url(PIONEX_HOST);
		std::string url = "/api/v1/trade/orderByClientOrderId?";
		std::string action = "GET";

		std::string tp = std::to_string(get_current_ms_epoch());
		std::string pre_data = std::string("clientOrderId=") + (localOrderId)+std::string("&symbol=") + (symbol)
			+std::string("&timestamp=") + tp;

		url += pre_data;

		std::vector < std::string> extra_http_header;
		std::string header_chunk("PIONEX-KEY: ");
		header_chunk.append(m_api_key);

		std::string header_SIGNATURE("PIONEX-SIGNATURE: ");
		std::string tot_str = action + url;
		std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
		header_SIGNATURE.append(signature);

		extra_http_header.push_back(header_chunk);
		extra_http_header.push_back(header_SIGNATURE);

		std::string str_result;
		std::string post_data = "";
		tot_url += url;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		if (str_result.size() > 0) {

			try {
				parse_string2json(str_result, json_result);
			}
			catch (std::exception& e) {
				SPDLOG_ERROR("PionexAccount::get_order:{}", e.what());
				return false;
			}
		}
		else {
			SPDLOG_ERROR("<PionexAccount::get_order> Failed to get anything.");
			return false;
		}
		return true;
	}

	bool get_all_pos(Json::Value& json_result)
	{
		std::string tot_url(PIONEX_HOST);
		std::string url = "/api/v1/account/balances?";
		std::string action = "GET";

		std::string tp = std::to_string(get_current_ms_epoch());
		std::string pre_data = std::string("timestamp=") + tp;

		url += pre_data;

		std::vector < std::string> extra_http_header;
		std::string header_chunk("PIONEX-KEY: ");
		header_chunk.append(m_api_key);

		std::string header_SIGNATURE("PIONEX-SIGNATURE: ");
		std::string tot_str = action + url;
		std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
		header_SIGNATURE.append(signature);

		extra_http_header.push_back(header_chunk);
		extra_http_header.push_back(header_SIGNATURE);

		std::string str_result;
		std::string post_data = "";
		tot_url += url;
		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		if (str_result.size() > 0) {

			try {
				parse_string2json(str_result, json_result);
			}
			catch (std::exception& e) {
				SPDLOG_ERROR("PionexAccount::get_all_pos:{}", e.what());
				return false;
			}
		}
		else {
			SPDLOG_ERROR("<PionexAccount::get_all_pos> Failed to get anything.");
			return false;
		}
		return true;
	} 
};

#endif