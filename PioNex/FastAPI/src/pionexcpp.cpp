#include "pionexcpp.h"
#include "binacpp_logger.h"
#include "decimal.h"

string PionexCPP::m_api_key = "";
string PionexCPP::m_secret_key = "";
CURL* PionexCPP::m_curl = NULL;

void PionexCPP::init(string &api_key, string &secret_key)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	PionexCPP::m_curl = curl_easy_init();
	PionexCPP::m_api_key = api_key;
	PionexCPP::m_secret_key = secret_key;
}

void PionexCPP::cleanup()
{
	curl_easy_cleanup(PionexCPP::m_curl);
	curl_global_cleanup();
}

bool PionexCPP::parse_string2json(std::string &str_result, Json::Value &json_result)
{
	Json::Reader reader;
	json_result.clear();
	bool res = reader.parse(str_result, json_result);
	return res;
}

void PionexCPP::send_order(const char *symbol, const char *side,
	const char *orderType, double qty, double price,
	const char* localOrderId, const char* spending,
	Json::Value &json_result)
{
	int sid = 0;
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}

	std::string url(PIONEX_HOST);
	url += "/tapi/v1/trade/order?";
	std::string action = "POST";

	std::string tp = std::to_string(get_current_ms_epoch());
	std::string pre_data = std::string("apiKey=") + m_api_key + std::string("&strategyId=") + std::to_string(sid)
		+ std::string("&timestamp=") + tp;
	std::string signature = get_pionex_trding_key(m_secret_key.c_str(), pre_data.c_str());

	std::string append = std::string("strategyId=") + std::to_string(sid) + std::string("&timestamp=") + tp
		+ std::string("&apiKey=") + m_api_key + std::string("&signature=") + signature;
	url += append;

	Json::Value jsObj;
	jsObj["symbol"] = symbol;
	jsObj["side"] = side;
	jsObj["orderType"] = orderType;
	jsObj["strategyId"] = sid;

	if (strcmp(orderType, "MARKET") != 0)
	{
		jsObj["qty"] = std::to_string(qty);
		dec::decimal<4> value(price);
		std::string sValue = dec::toString(value);
		jsObj["price"] = sValue;
	}
	else if (spending != nullptr)
	{
		jsObj["spending"] = spending;//�м۵�Ҫ���ѵı�
	}
	if (localOrderId != nullptr)
	{
		jsObj["localOrderId"] = localOrderId;
	}
	std::string post_data = jsObj.toStyledString();
	std::vector <std::string> extra_http_header;
	std::string str_result;

	curl_api_with_header(url, str_result, extra_http_header, post_data, action);

	if (str_result.size() > 0) {
		try {
			parse_string2json(str_result, json_result);
		}
		catch (exception &e) {
			BinaCPP_logger::write_log("<PionexCPP::send_order> Error ! %s", e.what());
		}
		BinaCPP_logger::write_log("<PionexCPP::send_order> Done.");

	}
	else {
		BinaCPP_logger::write_log("<PionexCPP::send_order> Failed to get anything.");
	}
	BinaCPP_logger::write_log("<PionexCPP::send_order:%s> Done.\n", symbol);
}

void PionexCPP::cancel_order(const char *symbol,long orderId,Json::Value &json_result)
{
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}
	int strategyId = 0;
	std::string url(PIONEX_HOST);
	url += "/tapi/v1/trade/order?";
	std::string action = "DELETE";

	std::string tp = std::to_string(get_current_ms_epoch());
	std::string pre_data = std::string("apiKey=") + m_api_key + std::string("&strategyId=") + std::to_string(strategyId)
		+ std::string("&timestamp=") + tp;
	std::string signature = get_pionex_trding_key(m_secret_key.c_str(), pre_data.c_str());

	std::string append = std::string("strategyId=") + std::to_string(strategyId) + std::string("&timestamp=") + tp
		+ std::string("&apiKey=") + m_api_key + std::string("&signature=") + signature;
	url += append;

	Json::Value jsObj;
	jsObj["symbol"] = symbol;
	jsObj["orderId"] = orderId;
	std::string post_data = jsObj.toStyledString();

	std::vector <std::string> extra_http_header;
	//BinaCPP_logger::write_log("<PionexCPP::send_order> url = |%s|, post_data = |%s|", url.c_str(), post_data.c_str());
	string str_result;
	curl_api_with_header(url, str_result, extra_http_header, post_data, action);

	if (str_result.size() > 0) {

		try {
			parse_string2json(str_result, json_result);
		}
		catch (exception &e) {
			BinaCPP_logger::write_log("<PionexCPP::cancel_order> Error ! %s", e.what());
		}
	}
	else {
		BinaCPP_logger::write_log("<PionexCPP::cancel_order> Failed to get anything.");
	}

	BinaCPP_logger::write_log("<PionexCPP::cancel_order:%d> Done.\n", orderId);
}

void PionexCPP::get_order(const char *symbol, long orderId, Json::Value &json_result)//orderID
{
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}
	int strategyId = 0;
	std::string url(PIONEX_HOST);
	url += "/tapi/v1/trade/order?";
	std::string action = "GET";

	std::string tp = std::to_string(get_current_ms_epoch());
	std::string pre_data = std::string("apiKey=") + m_api_key + std::string("&orderId=") + std::to_string(orderId) + std::string("&symbol=") + (symbol)
		+std::string("&timestamp=") + tp;
	std::string signature = get_pionex_trding_key(m_secret_key.c_str(), pre_data.c_str());

	std::string append = std::string("orderId=") + std::to_string(orderId) + std::string("&symbol=") + symbol + std::string("&timestamp=") + tp
		+ std::string("&apiKey=") + m_api_key + std::string("&signature=") + signature;
	url += append;

	std::vector <std::string> extra_http_header;
	std::string str_result;
	std::string post_data = "";
	curl_api_with_header(url, str_result, extra_http_header, post_data, action);

	if (str_result.size() > 0) {

		try {
			parse_string2json(str_result, json_result);
		}
		catch (exception &e) {
			BinaCPP_logger::write_log("<PionexCPP::get_order> Error ! %s", e.what());
		}
	}
	else {
		BinaCPP_logger::write_log("<PionexCPP::get_order> Failed to get anything.");
	}

	BinaCPP_logger::write_log("<PionexCPP::get_order:%d> Done.\n", orderId);
}

void PionexCPP::get_order(const char *symbol, const char* localOrderId, Json::Value &json_result)//localID
{
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}
	int strategyId = 0;
	std::string url(PIONEX_HOST);
	url += "/tapi/v1/trade/localOrder?";
	std::string action = "GET";

	std::string tp = std::to_string(get_current_ms_epoch());
	std::string pre_data = std::string("apiKey=") + m_api_key + std::string("&localOrderId=") + (localOrderId)+std::string("&symbol=") + (symbol)
		+std::string("&timestamp=") + tp;
	std::string signature = get_pionex_trding_key(m_secret_key.c_str(), pre_data.c_str());

	std::string append = std::string("localOrderId=") + localOrderId + std::string("&symbol=") + symbol + std::string("&timestamp=") + tp
		+ std::string("&apiKey=") + m_api_key + std::string("&signature=") + signature;
	url += append;

	std::vector < std::string> extra_http_header;
	std::string str_result;
	std::string post_data = "";
	curl_api_with_header(url, str_result, extra_http_header, post_data, action);

	if (str_result.size() > 0) {

		try {
			parse_string2json(str_result, json_result);
		}
		catch (exception &e) {
			BinaCPP_logger::write_log("<PionexCPP::get_order> Error ! %s", e.what());
		}
	}
	else {
		BinaCPP_logger::write_log("<PionexCPP::get_order> Failed to get anything.");
	}

	BinaCPP_logger::write_log("<PionexCPP::get_order:%s> Done.\n", localOrderId);
}

//-----------------
// Curl's callback
size_t PionexCPP::curl_cb(void *content, size_t size, size_t nmemb, std::string *buffer)
{
	//BinaCPP_logger::write_log("<PionexCPP::curl_cb> ");
	buffer->append((char*)content, size*nmemb);
	//BinaCPP_logger::write_log("<PionexCPP::curl_cb> done");
	return size * nmemb;
}

void PionexCPP::curl_api(string &url, string &result_json) {
	vector <string> v;
	string action = "GET";
	string post_data = "";
	curl_api_with_header(url, result_json, v, post_data, action);
}

//--------------------
// Do the m_curl
void PionexCPP::curl_api_with_header(string &url, string &str_result, vector <string> &extra_http_header, string &post_data, string &action)
{

	BinaCPP_logger::write_log("<PionexCPP::curl_api>");

	CURLcode res;

	if (PionexCPP::m_curl) {

		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_WRITEFUNCTION, PionexCPP::curl_cb);
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_WRITEDATA, &str_result);
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_ENCODING, "gzip");

		if (extra_http_header.size() > 0) {

			struct curl_slist *chunk = NULL;
			for (int i = 0; i < extra_http_header.size(); i++) {
				chunk = curl_slist_append(chunk, extra_http_header[i].c_str());
			}
			curl_easy_setopt(PionexCPP::m_curl, CURLOPT_HTTPHEADER, chunk);
		}

		if (post_data.size() > 0 || action == "POST" || action == "PUT" || action == "DELETE") {

			if (action == "PUT" || action == "DELETE") {
				curl_easy_setopt(PionexCPP::m_curl, CURLOPT_CUSTOMREQUEST, action.c_str());
			}
			curl_easy_setopt(PionexCPP::m_curl, CURLOPT_POSTFIELDS, post_data.c_str());
		}

		res = curl_easy_perform(PionexCPP::m_curl);

		/* Check for errors */
		if (res != CURLE_OK) {
			BinaCPP_logger::write_log("<PionexCPP::curl_api> curl_easy_perform() failed: %s", curl_easy_strerror(res));
		}
	}
	//BinaCPP_logger::write_log("<PionexCPP::curl_api> done");
}
