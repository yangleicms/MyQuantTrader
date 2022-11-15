#include "pionexcpp.h"
#include "binacpp_logger.h"
#include "decimal.h"
#include "utils.h"
#include <iostream>

#define DEBUG
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
#ifdef DEBUG
	std::cout<<str_result<<std::endl;
#endif
	bool res = reader.parse(str_result, json_result);
	return res;
}

std::string PionexCPP::get_pionex_private_url() {

	std::string tp = std::to_string(pionex_get_current_ms_epoch());
	std::string data = "/ws?key=" + m_api_key + "&timestamp=" + tp + "websocket_auth";
	std::string signature = hmac_sha256(m_secret_key.c_str(), data.c_str());
	std::string url = "/ws?key=" + m_api_key + "&timestamp=" + tp + "&signature=" + signature;
	return url;
}

void PionexCPP::sub_order_ws(std::string instrument,int cli_private_index)
{
	auto private_cli = Webclient::get_cli(cli_private_index);
	if (private_cli == nullptr) {
		std::cout << "can not find WebSocketSSLClient:" << cli_private_index << std::endl;
		return;
	}
	Json::Value jsObj;
	jsObj["op"] = "SUBSCRIBE";
	jsObj["topic"] = "ORDER";
	jsObj["symbol"] = instrument.data();
	std::string jsonstr = jsObj.toStyledString();
	private_cli->Send(jsonstr);
}

void PionexCPP::sub_fill_ws(std::string instrument, int cli_private_index)
{
	auto private_cli = Webclient::get_cli(cli_private_index);
	if (private_cli == nullptr) {
		std::cout << "can not find WebSocketSSLClient:" << cli_private_index << std::endl;
		return;
	}
	Json::Value fillObj;
	fillObj["op"] = "SUBSCRIBE";
	fillObj["topic"] = "FILL";
	fillObj["symbol"] = instrument.data();
	std::string str = fillObj.toStyledString();
	private_cli->Send(str);
}

void PionexCPP::sub_depth(std::string instrument, int cli_public_index)
{
	auto pub_cli = Webclient::get_cli(cli_public_index);
	if (pub_cli == nullptr) {
		std::cout << "can not find WebSocketSSLClient:" << cli_public_index << std::endl;
		return;
	}
	Json::Value jsObj;
	jsObj["op"] = "SUBSCRIBE";
	jsObj["topic"] = "DEPTH";
	jsObj["symbol"] = instrument.data();
	jsObj["limit"] = 5;
	std::string jsonstr = jsObj.toStyledString();
	pub_cli->Send(jsonstr);
}

void PionexCPP::send_order(const char *symbol, const char *side,
	const char *type,const char* clientOrderId,double size,
	double price,double amount,bool IOC,Json::Value &json_result)
{
	int sid = 0;
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}

	std::string tot_url(PIONEX_HOST);
	std::string url = "/api/v1/trade/order?";
	std::string action = "POST";

	std::string tp = std::to_string(pionex_get_current_ms_epoch());
	std::string pre_data = std::string("timestamp=") + tp;
	url += pre_data;
	//std::string signature = get_pionex_trding_key(m_secret_key.c_str(), pre_data.c_str());

	Json::Value jsObj;
	jsObj["symbol"] = symbol;
	jsObj["side"] = side;
	jsObj["type"] = type;

	if (strcmp(type, "MARKET") != 0)
	{
		dec::decimal<4> qty(size);
		jsObj["size"] = dec::toString(qty);
		dec::decimal<4> value(price);
		std::string sValue = dec::toString(value);
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
	string header_chunk("PIONEX-KEY: ");
	header_chunk.append(m_api_key);
	
	string header_SIGNATURE("PIONEX-SIGNATURE: ");
	string tot_str = action + url + post_data;
	//std::string signature = get_pionex_trding_key(m_secret_key.c_str(), tot_str.c_str());
	std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
	header_SIGNATURE.append(signature);

	extra_http_header.push_back(header_chunk);
	extra_http_header.push_back(header_SIGNATURE);

	std::string str_result;
	tot_url += url;
	//curl_api_with_header(tot_url, str_result, extra_http_header, post_data, action);
	getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
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

void PionexCPP::cancel_order(const char *symbol,uint64_t orderId,Json::Value &json_result)
{
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}

	std::string tot_url(PIONEX_HOST);
	std::string url = "/api/v1/trade/order?";
	std::string action = "DELETE";

	std::string tp = std::to_string(pionex_get_current_ms_epoch());
	std::string pre_data = std::string("timestamp=") + tp;
	url += pre_data;

	Json::Value jsObj;
	jsObj["symbol"] = symbol;
	jsObj["orderId"] = orderId;
	std::string post_data = jsObj.toStyledString();

	std::vector <std::string> extra_http_header;
	string header_chunk("PIONEX-KEY: ");
	header_chunk.append(m_api_key);

	string header_SIGNATURE("PIONEX-SIGNATURE: ");
	string tot_str = action + url + post_data;
	std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
	header_SIGNATURE.append(signature);

	extra_http_header.push_back(header_chunk);
	extra_http_header.push_back(header_SIGNATURE);
	
	string str_result;
	tot_url += url;
	//curl_api_with_header(tot_url, str_result, extra_http_header, post_data, action);
	getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
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

	BinaCPP_logger::write_log("<PionexCPP::cancel_order:%lld> Done.\n", orderId);
}

void PionexCPP::get_order(const char *symbol, uint64_t orderId, Json::Value &json_result)//orderID
{
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::send_order> API Key and Secret Key has not been set.");
		return;
	}
	
	std::string tot_url(PIONEX_HOST);
	std::string url = "/api/v1/trade/order?";
	std::string action = "GET";

	std::string tp = std::to_string(pionex_get_current_ms_epoch());
	std::string pre_data = std::string("orderId=") + std::to_string(orderId) + std::string("&symbol=") + (symbol)
		+std::string("&timestamp=") + tp;

	url += pre_data;

	std::vector <std::string> extra_http_header;
	std::string header_chunk("PIONEX-KEY: ");
	header_chunk.append(m_api_key);

	string header_SIGNATURE("PIONEX-SIGNATURE: ");
	string tot_str = action + url;
	std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
	header_SIGNATURE.append(signature);

	extra_http_header.push_back(header_chunk); 
	extra_http_header.push_back(header_SIGNATURE);

	std::string str_result;
	std::string post_data = "";
	tot_url += url;
	//std::cout<<"getOrderBySysID,url:"<<tot_url<<",method:"<<action<<",PIONEX-SIGNATURE:"<<signature<<",PIONEX-KEY:"<<m_api_key<<std::endl;;
	//curl_api_with_header(tot_url, str_result, extra_http_header, post_data, action);
	getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
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

	BinaCPP_logger::write_log("<PionexCPP::get_order:%lld> Done.\n", orderId);
}

void PionexCPP::get_order(const char *symbol, const char* localOrderId, Json::Value &json_result)//localID
{
	if (m_api_key.size() == 0 || m_secret_key.size() == 0) {
		BinaCPP_logger::write_log("<PionexCPP::get_order> API Key and Secret Key has not been set.");
		return;
	}

	std::string tot_url(PIONEX_HOST);
	std::string url = "/api/v1/trade/orderByClientOrderId?";
	std::string action = "GET";

	std::string tp = std::to_string(pionex_get_current_ms_epoch());
	std::string pre_data = std::string("clientOrderId=") + (localOrderId)+std::string("&symbol=") + (symbol)
		+std::string("&timestamp=") + tp;
	
	url += pre_data;

	std::vector < std::string> extra_http_header;
	std::string header_chunk("PIONEX-KEY: ");
	header_chunk.append(m_api_key);

	string header_SIGNATURE("PIONEX-SIGNATURE: ");
	string tot_str = action + url;
	std::string signature = hmac_sha256(m_secret_key.c_str(), tot_str.c_str());
	header_SIGNATURE.append(signature);

	extra_http_header.push_back(header_chunk);
	extra_http_header.push_back(header_SIGNATURE);

	std::string str_result;
	std::string post_data = "";
	tot_url += url;
	//std::cout<<"getOrderByOrderID,url:"<<tot_url<<",method:"<<action<<",PIONEX-SIGNATURE:"<<signature<<",PIONEX-KEY:"<<m_api_key<<std::endl;
	//curl_api_with_header(tot_url, str_result, extra_http_header, post_data, action);
	getCurlWithHeader(str_result, tot_url, extra_http_header, post_data, action);
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

void PionexCPP::get_all_pos(Json::Value& json_result)
{
	std::string tot_url(PIONEX_HOST);
	std::string url = "/api/v1/trade/order?";
	std::string action = "GET";

	std::string tp = std::to_string(pionex_get_current_ms_epoch());
	std::string pre_data = std::string("&timestamp=") + tp;

	url += pre_data;

	std::vector < std::string> extra_http_header;
	std::string header_chunk("PIONEX-KEY: ");
	header_chunk.append(m_api_key);

	string header_SIGNATURE("PIONEX-SIGNATURE: ");
	string tot_str = action + url;
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
		catch (exception& e) {
			BinaCPP_logger::write_log("<PionexCPP::get_all_pos> Error ! %s", e.what());
		}
	}
	else {
		BinaCPP_logger::write_log("<PionexCPP::get_all_pos> Failed to get anything.");
	}

	BinaCPP_logger::write_log("<PionexCPP::get_all_pos:%s> Done.\n");
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
	//BinaCPP_logger::write_log("<PionexCPP::curl_api>");
	//std::cout<<url<<std::endl;
	CURLcode res;

	if (PionexCPP::m_curl) {

		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_WRITEFUNCTION, PionexCPP::curl_cb);
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_WRITEDATA, &str_result);
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_SSL_VERIFYPEER, false);
		//curl_easy_setopt(PionexCPP::m_curl, CURLOPT_ENCODING, "gzip");

		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
		curl_easy_setopt(PionexCPP::m_curl, CURLOPT_HTTPHEADER, headers);

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

