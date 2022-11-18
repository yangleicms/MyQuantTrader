#pragma once
#ifndef OKEX_API_H
#define OKEX_API_H
#include "base_websocket.h"
#include "common.h"
#include "cell_log.h"
#include "msg.h"

using namespace cell::common;
#define OKEX_HOST "https://www.okx.com"
#define OKEX_WS_HOST "ws.okx.com"

#define OKEX_WS_PUB_PATH "/ws/v5/public"
#define OKEX_WS_PRI_PATH "/ws/v5/private"

//#define DEBUG
const int okex_port = 8443;
const int okex_sendOrder = 1024;
const int okex_cancelOrder = 2000;

static void okex_parse_json(Json::Value& json_result, std::string& str_result)
{
	try
	{
#ifdef DEBUG
		std::cout << str_result << std::endl;
#endif
		Json::CharReaderBuilder b;
		std::unique_ptr<Json::CharReader> reader(b.newCharReader());
		json_result.clear();
		JSONCPP_STRING errs;
		reader->parse(str_result.data(), str_result.data() + strlen(str_result.data()), &json_result, &errs);
	}
	catch (std::exception& e)
	{
		SPDLOG_ERROR("<get_account> Error ! %s", e.what());

	}
}

static std::string get_okex_sign(const char* key, const char* data)
{
	unsigned char digest[32];
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
		reinterpret_cast<const unsigned char*>(key), strlen(key),
		reinterpret_cast<const unsigned char*>(data), strlen(data),
		digest);
	std::string b64 = base64_encode((const char*)digest, 32);

	return b64;
}

class OkexAccount
{
public:
	std::string m_api_key, m_secret_key, m_pass;
	std::shared_ptr<WebSocketSSLClient> wscli;
public:

	OkexAccount(const std::string api_key, const std::string secret_key, std::string passwd)
	{
		m_api_key = api_key;
		m_secret_key = secret_key;
		m_pass = passwd;
	}

	bool keysAreSet()
	{
		return ((m_api_key.size() == 0) || (m_secret_key.size() == 0) || m_pass.size() == 0);
	}

	std::string get_utc_time()
	{
		struct tm* local;
		time_t t;
		t = time(NULL);
		local = gmtime(&t);

		int year = local->tm_year + 1900;
		int mon = local->tm_mon + 1;
		int day = local->tm_mday;
		int hour = local->tm_hour;
		int min = local->tm_min;
		int sec = local->tm_sec;

		char buf[32] = { 0 };
		sprintf(buf, "%d-%02d-%02dT%02d:%02d:%02d", year, mon, day, hour, min, sec);
		std::string res = buf;
		res += ".500Z";
		return res;
	}

	int send_lws_msg(const char* str, int len)
	{
		std::string strr = std::string(str, len);
		wscli->Send(strr);
		return 0;
	}

	void send_order_ws(const TBCryptoInputField* input)
	{
		Json::Value jsObj, sub, args;
		jsObj["id"] = std::to_string(okex_sendOrder);
		jsObj["op"] = "order";

		if (input->Direction == TD_LONG_DIRECTION)
			sub["side"] = "buy";
		else
			sub["side"] = "sell";
		sub["instId"] = input->InstrumentID;
		sub["tdMode"] = "cross";

		if (TD_PriceType_LimitPrice == input->PriceType)
			sub["ordType"] = "limit";
		else if (TD_PriceType_AnyPrice == input->PriceType)
			sub["ordType"] = "market";
		else if (TD_PriceType_LimitMarker == input->PriceType)
			sub["ordType"] = "post_only";

		if (TD_TimeCondition_IOC == input->TimeCondition) {
			if (TD_VolumeCondition_CV == input->VolumeCondition)
				sub["ordType"] = "fok";
			else
				sub["ordType"] = "ioc";
		}

		sub["clOrdId"] = std::to_string(input->key);
		sub["sz"] = toString(input->VolumeTotal);
		sub["px"] = toString(input->LimitPrice);

		args.append(sub);
		jsObj["args"] = args;

		std::string jsonstr = jsObj.toStyledString();
		//std::cout << jsonstr << std::endl;
		wscli->Send(jsonstr);
	}

	void cancel_order_ws(const char* InstrumentID, const char* orderID)
	{
		Json::Value jsObj, sub, args;
		jsObj["id"] = std::to_string(okex_cancelOrder);
		jsObj["op"] = "cancel-order";

		sub["instId"] = InstrumentID;
		sub["ordId"] = orderID;
		args.append(sub);

		jsObj["args"] = args;

		std::string jsonstr = jsObj.toStyledString();
		//std::cout << jsonstr << std::endl;
		wscli->Send(jsonstr);
	}

	void send_order(const TBCryptoInputField* input, Json::Value& json_result)
	{
		Json::Value sub;
		if(input->Direction == TD_LONG_DIRECTION)
			sub["side"] = "buy";
		else
			sub["side"] = "sell";
		sub["instId"] = input->InstrumentID;
		if(input->CryptoType==CRYPTO_SPOT){
			sub["tdMode"] = "cash";
		}
		else{
			sub["tdMode"] = "cross";
			sub["ccy"] = "USDT";
		}

		if (TD_PriceType_LimitPrice == input->PriceType)
			sub["ordType"] = "limit";
		else if (TD_PriceType_AnyPrice == input->PriceType)
			sub["ordType"] = "market";
		else if (TD_PriceType_LimitMarker == input->PriceType)
			sub["ordType"] = "post_only";

		if (TD_TimeCondition_IOC == input->TimeCondition) {
			if (TD_VolumeCondition_CV == input->VolumeCondition)
				sub["ordType"] = "fok";
			else
				sub["ordType"] = "ioc";
		}

		sub["clOrdId"] = std::to_string(input->key);
		sub["sz"] = toString(input->VolumeTotal);
		if (strstr(input->InstrumentID, "SWAP") != NULL) {
			sub["sz"] = std::to_string((int)(input->VolumeTotal + 0.5));
		}
		sub["px"] = toString(input->LimitPrice);

		std::string post_data = sub.toStyledString();

		std::string tot_url(OKEX_HOST);
		std::string url = "/api/v5/trade/order";
		std::string action = "POST";

		std::vector <std::string> extra_http_header;
		std::string key = "OK-ACCESS-KEY:" + m_api_key;
		extra_http_header.push_back(key);

		std::string tp = get_utc_time();
		key = "OK-ACCESS-TIMESTAMP:" + tp;
		extra_http_header.push_back(key);

		key = "OK-ACCESS-PASSPHRASE:" + m_pass;
		extra_http_header.push_back(key);

		std::string sign_str = tp + "POST" + "/api/v5/trade/order"+post_data;
		std::string sign = get_okex_sign(m_secret_key.data(), sign_str.data());
		key = "OK-ACCESS-SIGN:" + sign;
		extra_http_header.push_back(key);

		key = "Content-Type:application/json";
		extra_http_header.push_back(key);

		std::string str_result;
		tot_url += url;
		
		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		//std::cout<<str_result<<std::endl;
		if (str_result.size() > 0) {
			try {
				okex_parse_json(json_result, str_result);
			}
			catch (std::exception& e) {

			}
		}
		else {

		}
	}

	void cancel_order(const char* InstrumentID, const char* orderID,int64_t clOrdId, Json::Value& json_result)
	{
		Json::Value sub;
		sub["instId"] = InstrumentID;
		if (strlen(orderID) > 0)
			sub["ordId"] = orderID;
		else
			sub["clOrdId"] = clOrdId;

		std::string post_data = sub.toStyledString();

		std::string tot_url(OKEX_HOST);
		std::string url = "/api/v5/trade/cancel-order";
		std::string action = "POST";

		std::vector <std::string> extra_http_header;
		std::string key = "OK-ACCESS-KEY:" + m_api_key;
		extra_http_header.push_back(key);

		std::string tp = get_utc_time();
		key = "OK-ACCESS-TIMESTAMP:" + tp;
		extra_http_header.push_back(key);

		key = "OK-ACCESS-PASSPHRASE:" + m_pass;
		extra_http_header.push_back(key);

		std::string sign_str = tp + "POST" + "/api/v5/trade/cancel-order" + post_data;
		std::string sign = get_okex_sign(m_secret_key.data(), sign_str.data());
		key = "OK-ACCESS-SIGN:" + sign;
		extra_http_header.push_back(key);

		key = "Content-Type:application/json";
		extra_http_header.push_back(key);

		std::string str_result;
		tot_url += url;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		//std::cout<<str_result<<std::endl;
		if (str_result.size() > 0) {
			try {
				okex_parse_json(json_result, str_result);
			}
			catch (std::exception& e) {

			}
		}
		else {

		}
	}

	void get_pos(Json::Value& json_result)
	{
		std::string tot_url(OKEX_HOST);
		std::string url = "/api/v5/account/positions";
		std::string action = "GET";

		std::vector <std::string> extra_http_header;
		std::string key = "OK-ACCESS-KEY:" + m_api_key;
		extra_http_header.push_back(key);

		std::string tp = get_utc_time();
		key = "OK-ACCESS-TIMESTAMP:" + tp;
		extra_http_header.push_back(key);

		key = "OK-ACCESS-PASSPHRASE:" + m_pass;
		extra_http_header.push_back(key);

		std::string sign_str = tp + "GET" + "/api/v5/account/positions";
		std::string sign = get_okex_sign(m_secret_key.data(), sign_str.data());
		key = "OK-ACCESS-SIGN:" + sign;
		extra_http_header.push_back(key);

		std::string str_result;
		tot_url += url;
		std::string post_data;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		//std::cout<<str_result<<std::endl;
		if (str_result.size() > 0) {
			try {
				okex_parse_json(json_result, str_result);
			}
			catch (std::exception& e) {

			}
		}
		else {

		}
	}

	void get_spot_pos(Json::Value& json_result) 
	{
		std::string tot_url(OKEX_HOST);
		std::string url = "/api/v5/account/balance";
		std::string action = "GET";

		std::vector <std::string> extra_http_header;
		std::string key = "OK-ACCESS-KEY:" + m_api_key;
		extra_http_header.push_back(key);

		std::string tp = get_utc_time();
		key = "OK-ACCESS-TIMESTAMP:" + tp;
		extra_http_header.push_back(key);

		key = "OK-ACCESS-PASSPHRASE:" + m_pass;
		extra_http_header.push_back(key);

		std::string sign_str = tp + "GET" + "/api/v5/account/balance";
		std::string sign = get_okex_sign(m_secret_key.data(), sign_str.data());
		key = "OK-ACCESS-SIGN:" + sign;
		extra_http_header.push_back(key);

		std::string str_result;
		tot_url += url;
		std::string post_data;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		//std::cout<<str_result<<std::endl;
		if (str_result.size() > 0) {
			try {
				okex_parse_json(json_result, str_result);
			}
			catch (std::exception& e) {

			}
		}
		else {

		}
		
	}

	void get_order(Json::Value& json_result, const char* InstrumentID, const char* LocalID, const char* SysID)
	{
		std::string tot_url(OKEX_HOST);
		std::string path = std::string("/api/v5/trade/order?instId=") + InstrumentID;
		std::string action = "GET";

		if (strlen(LocalID) > 0) {
			path += (std::string("&clOrdId=") + LocalID);
		}
		if (strlen(SysID) > 0) {
			path += (std::string("&ordId=") + SysID);
		}

		std::vector <std::string> extra_http_header;
		std::string key = "OK-ACCESS-KEY:" + m_api_key;
		extra_http_header.push_back(key);

		std::string tp = get_utc_time();
		key = "OK-ACCESS-TIMESTAMP:" + tp;
		extra_http_header.push_back(key);

		key = "OK-ACCESS-PASSPHRASE:" + m_pass;
		extra_http_header.push_back(key);

		std::string sign_str = tp + "GET" + path;
		std::string sign = get_okex_sign(m_secret_key.data(), sign_str.data());
		key = "OK-ACCESS-SIGN:" + sign;
		extra_http_header.push_back(key);

		std::string str_result;
		tot_url += path;
		std::string post_data;

		getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
		if (str_result.size() > 0) {
			try {
				okex_parse_json(json_result, str_result);
			}
			catch (std::exception& e) {

			}
		}
		else {

		}
	}
};

#endif
