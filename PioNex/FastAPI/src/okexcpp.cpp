#include "okexcpp.h"
#include "binacpp_logger.h"
#include "decimal.h"
#include "utils.h"
#include <iostream>

#define DEBUG
string OkexCPP::m_api_key = "";
string OkexCPP::m_secret_key = "";
string OkexCPP::m_pass = "";
CURL* OkexCPP::m_curl = NULL;


void OkexCPP::init(string& api_key, string& secret_key,string & passwd)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	OkexCPP::m_curl = curl_easy_init();
	OkexCPP::m_api_key = api_key;
	OkexCPP::m_secret_key = secret_key;
	OkexCPP::m_pass = passwd;
}

void OkexCPP::cleanup()
{
	curl_easy_cleanup(OkexCPP::m_curl);
	curl_global_cleanup();
}

bool OkexCPP::parse_string2json(std::string& str_result, Json::Value& json_result)
{
	Json::Reader reader;
	json_result.clear();
#ifdef DEBUG
	std::cout << str_result << std::endl;
#endif
	bool res = reader.parse(str_result, json_result);
	return res;
}

void OkexCPP::send_order(const TBCryptoInputField* input, std::function<void(std::string&, std::string&)> callback)
{
	Json::Value sub;
	if (input->Direction == 0)
		sub["side"] = "buy";
	else
		sub["side"] = "sell";
	sub["instId"] = input->InstrumentID;
	if (input->CryptoType == 0) {
		sub["tdMode"] = "cash";
	}
	else {
		sub["tdMode"] = "cross";
		sub["ccy"] = "USDT";
	}

	if (0 == input->PriceType)
		sub["ordType"] = "limit";
	else if (0 == input->PriceType)
		sub["ordType"] = "market";
	else if (1 == input->PriceType)
		sub["ordType"] = "post_only";

	if (0 == input->TimeCondition) {
		if (0 == input->VolumeCondition)
			sub["ordType"] = "fok";
		else
			sub["ordType"] = "ioc";
	}

	sub["clOrdId"] = std::to_string(input->key);
	sub["sz"] = std::to_string(input->VolumeTotal);
	if (strstr(input->InstrumentID, "SWAP") != NULL) {
		sub["sz"] = std::to_string((int)(input->VolumeTotal + 0.5));
	}
	sub["px"] = std::to_string(input->LimitPrice);

	std::string post_data = sub.toStyledString();

	std::string tot_url(OKEX_HOST);
	std::string url = "/api/v5/trade/order";
	std::string action = "POST";

	std::map <std::string,std::string> extra_http_header;
	extra_http_header["OK-ACCESS-KEY"]= m_api_key;

	std::string tp = get_utc_time();
	extra_http_header["OK-ACCESS-TIMESTAMP"] = tp;

	extra_http_header["OK-ACCESS-PASSPHRASE"] = m_pass;

	std::string str_result;
	tot_url += url;

	http_async(tot_url, extra_http_header, post_data, action, std::to_string(input->key), callback);
}

void OkexCPP::get_pos() 
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
	std::string sign = pionex_fastAPI::get_okex_sign(m_secret_key.data(), sign_str.data());
	key = "OK-ACCESS-SIGN:" + sign;
	extra_http_header.push_back(key);

	std::string str_result;
	tot_url += url;
	std::string post_data;
	Json::Value json_result;

	getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
	if (str_result.size() > 0) {
		try {
			parse_string2json(str_result, json_result);
		}
		catch (exception& e) {
			BinaCPP_logger::write_log("<OkexCPP::get_pos> Error ! %s", e.what());
		}
		BinaCPP_logger::write_log("<OkexCPP::get_pos> Done.");

	}
	else {
		BinaCPP_logger::write_log("<OkexCPP::get_pos> Failed to get anything.");
	}
	

}

void OkexCPP::get_order(Json::Value& json_result, const char* InstrumentID, const char* LocalID, const char* SysID)
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
	std::string sign = pionex_fastAPI::get_okex_sign(m_secret_key.data(), sign_str.data());
	key = "OK-ACCESS-SIGN:" + sign;
	extra_http_header.push_back(key);

	std::string str_result;
	tot_url += path;
	std::string post_data;

	getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
	if (str_result.size() > 0) {
		try {
			parse_string2json(str_result,json_result);
		}
		catch (std::exception& e) {

		}
	}
	else {

	}
}
