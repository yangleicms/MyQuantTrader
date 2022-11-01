#pragma once

#pragma once

/*
	Author: yanglei
	Date  : 2022/10/12
	C++ library for Pionex API.
*/

#ifndef OKEXCPP_H
#define OKEXCPP_H


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
#include "utils.h"
#include "base_websocket.h"

#define OKEX_HOST "https://www.okx.com"

using namespace std;

class OkexCPP
{
	static string m_api_key;
	static string m_secret_key;
	static string m_pass;
	static CURL* m_curl;

public:
	static bool parse_string2json(std::string& str_result, Json::Value& json_result);
	static void init(string& api_key, string& secret_key, string& passwd);
	static void cleanup();
	static void get_pos();

	static void sub_order_ws(std::string instrument, int cli_private_index) {};

	static void send_order(const char* symbol, const char* side,
		const char* type, const char* clientOrderId, double size,
		double price, double amount, bool IOC, Json::Value& json_result) {};

	static void cancel_order(const char* symbol, uint64_t orderId, Json::Value& json_result) {};
	static void get_order(const char* symbol, uint64_t orderId, Json::Value& json_result) {};//orderID
	static void get_order(const char* symbol, const char* localOrderId, Json::Value& json_result) {};//localID

public:
	int m_strategy_id = 0;
};


#endif
