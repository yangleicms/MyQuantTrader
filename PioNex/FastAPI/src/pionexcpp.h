#pragma once

/*
	Author: yanglei
	Date  : 2022/10/12
	C++ library for Pionex API.
*/

#ifndef PIONEXCPP_H
#define PIONEXCPP_H


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <exception>

#include <curl/curl.h>
#include <json/json.h>
#include "pionex_util.h"
#include "base_websocket.h"

#define OLD_PIONEX_HOST "https://exchangedev.bthub.com/tapi"
#define PIONEX_HOST "https://api.pionex.com"

using namespace std;
using namespace pionex_fastAPI;

class PionexCPP 
{
	static string m_api_key;
	static string m_secret_key;
	static CURL* m_curl;

public:

	static void curl_api(string &url, string &result_json);
	static void curl_api_with_header(string &url, string &result_json, vector <string> &extra_http_header, string &post_data, string &action);
	static size_t curl_cb(void *content, size_t size, size_t nmemb, string *buffer);
	static bool parse_string2json(std::string &str_result, Json::Value &json_result);
	static std::string get_pionex_private_url();

	static void init(string &api_key, string &secret_key);
	static void cleanup();

	static void sub_order_ws(std::string instrument, int cli_private_index);
	static void sub_fill_ws(std::string instrument, int cli_private_index);
	static void connect_pionex_PubAndPrivate_ws(CB pub, CB pri, int& cli_public_index, int& cli_private_index);
	static void sub_depth(std::string instrument, int cli_public_index);

	static void send_order(const char *symbol, const char *side,
		const char *type, const char* clientOrderId, double size,
		double price, double amount, bool IOC, Json::Value &json_result);

	static void cancel_order(const char *symbol,uint64_t orderId,Json::Value &json_result);
	static void get_order(const char *symbol,uint64_t orderId, Json::Value &json_result);//orderID
	static void get_order(const char *symbol, const char* localOrderId, Json::Value &json_result);//localID
	
public:
	int m_strategy_id = 0;
};
#endif
