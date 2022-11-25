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

struct TBCryptoInputField
{
	struct {
		int64_t key;//orderID
		double LimitPrice = 0;
		double VolumeTotal = 0;
		char InstrumentID[19] = { 0 };

		uint8_t CryptoType;
		uint32_t SourceStrategyID = 0;//pubSub框架下，策略的源ID
		uint64_t pubSubKey = 0;//通过pubsub下单时，策略可以通过该成员来标识一个下单;
		uint64_t TradeItemHashVal = 0;
		uint32_t Tid = 0;//td_id
		uint32_t StrategyID = 0;//非pubSub框架直接对接trader的straID,pubsub框架下执行器也有这样一个ID

		//下列数据结构定义见data_type.h
		unsigned int ActionType : 6;//下单/撤单
		unsigned int Direction : 2;//交易方向
		unsigned int PriceType : 4;//价格类型，比如限价，市价
		unsigned int HedgeFlag : 2;
		unsigned int TimeCondition : 3;//有效时间类型，可能是GFD或者IOC
		unsigned int VolumeCondition : 3;//成交条件类型，可以是任意数量也可以是全部数量
		unsigned int OffsetFlag : 4;
		unsigned int PbFlag : 2;
		unsigned int ExchangeIdx : 8;//交易所ID
		unsigned int SubAccIdx : 6;
	};
};

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

	static void send_order(const TBCryptoInputField* input, std::function<void(std::string&, std::string&)> callback);

	static void cancel_order(const char* symbol, uint64_t orderId, Json::Value& json_result) {};
	static void get_order(Json::Value& json_result, const char* InstrumentID, const char* LocalID, const char* SysID);

public:
	int m_strategy_id = 0;
};


#endif
