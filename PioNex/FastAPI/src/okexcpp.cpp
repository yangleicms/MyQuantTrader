#include "okexcpp.h"
#include "binacpp_logger.h"
#include "decimal.h"
#include "utils.h"
#include <iostream>

#define DEBUG
string OkexCPP::m_api_key = "";
string OkexCPP::m_secret_key = "";
CURL* OkexCPP::m_curl = NULL;


void OkexCPP::init(string& api_key, string& secret_key)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	OkexCPP::m_curl = curl_easy_init();
	OkexCPP::m_api_key = api_key;
	OkexCPP::m_secret_key = secret_key;
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

void OkexCPP::get_pos() 
{
	std::string tot_url(OKEX_HOST);
	std::string url = "/api/v5/account/positions";
	std::string action = "GET";

	std::string tp = get_utc_time();

}