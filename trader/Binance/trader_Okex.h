#pragma once
#ifndef TRADE_OKEX_H
#define TRADE_OKEX_H

#include "Okex_api.h"
#include "crypto_trader.h"

using namespace cell::broker;

static int okex_rtn_event(Json::Value& json_result);

class trader_Okex : public crypto_trader
{
public:
	trader_Okex(const std::string td_source_name, const int td_id);
	virtual ~trader_Okex() {
		if (m_acc != nullptr) {
			delete m_acc;
		}
	};

public:
	//api
	virtual bool init(Json::Value conf)override;
	bool insert_order(const TBCryptoInputField* InputOrder)override;
	bool cancel_order(const TBCryptoInputField* ActionOrder)override;
	bool get_order(Order& order);//交易时刻查询
	void query_all_pos();

public:
	//spi
	void OnRtnOrder(Json::Value& json_result);
	void OnRtnTrade(Json::Value& json_result, Order* ord);
	
	void OnRspOrderInsert(Order* ord, std::string &errorID, std::string &msg);
	void OnRspOrderAction(Order* ord, std::string &errorID, std::string &msg);
	void OnRspOrderInsert(const TBCryptoInputField* InputOrder, std::string &errorID, std::string &msg);
	void OnRspOrderAction(const TBCryptoInputField* ActionOrder, std::string &errorID, std::string &msg);

	void OnWsReconn();
public:
	//tool func
	void thread_start_websocket();
	void thread_keep_alive();
	void login_ws();
	void sub_order_channel(std::string strr);
	int OrderStatus2TD(std::string& os);
	
	std::thread m_keep_thread;
	std::thread m_websocket_thread;
	bool m_PositionStatus = false;
	bool m_WebSocketInit = false;
	
	std::unordered_map<std::string, int>m_OKOS2TB;

private:
	std::string m_apiKey;
	std::string m_secKey;
	std::string m_passwd;

	std::string m_ws_subKey;

	OkexAccount* m_acc=nullptr;
};
#endif