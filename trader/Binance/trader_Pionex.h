#pragma once
#ifndef TRADE_PIONEX_H
#define TRADE_PIONEX_H

#include "Pionex_api.h"
#include "crypto_trader.h"

using namespace cell::broker;

class trader_Pionex : public crypto_trader
{
public:
	trader_Pionex(const std::string td_source_name, const int td_id);
	virtual ~trader_Pionex() {
		if (m_acc != nullptr) {
			delete m_acc;
		}
	};

public:
	//api
	virtual bool init(Json::Value conf)override;
	bool insert_order(const TBCryptoInputField* InputOrder)override;
	bool cancel_order(const TBCryptoInputField* ActionOrder)override;
	bool get_order(Order& order) { return true; };//交易时刻查询
	void query_all_pos();

public:
	//spi
	void OnRtnOrder(Json::Value& json_result);
	void OnRtnTrade(Json::Value& json_result);

	void OnRspOrderInsert(Order* ord, int errorID, std::string& msg);
	void OnRspOrderAction(Order* ord, int errorID, std::string& msg);
	void OnRspOrderInsert(const TBCryptoInputField* InputOrder, int errorID, std::string& msg);
	void OnRspOrderAction(const TBCryptoInputField* ActionOrder, int errorID, std::string& msg);

public:
	//tool func
	void thread_start_websocket();
	void sub_order_channel(std::string strr);
	//int OrderStatus2TD(std::string& os);

	int pionex_order_fill_event(Json::Value& json_result);
	void OnPing(std::string& js);

	std::thread m_keep_thread;
	std::thread m_websocket_thread;
	bool m_PositionStatus = false;
	bool m_WebSocketInit = false;

	std::unordered_map<std::string, int>m_OS2TD;
	std::unordered_map<std::string, int64_t>m_sysID2localID;
	PionexAccount* m_acc = nullptr;

private:
	std::string m_apiKey;
	std::string m_secKey;

	
};


#endif
