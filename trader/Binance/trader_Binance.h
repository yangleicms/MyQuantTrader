#pragma once
#ifndef TRADE_BINANCE_H
#define TRADE_BINANCE_H

#include "Binance_api.h"
#include "crypto_trader.h"

using namespace cell::broker;

static int binance_order_event(Json::Value& json_result);
static int binance_order_event_fut(Json::Value& json_result);

typedef struct __accInfo
{
	BinanceAccount* cash = nullptr;
	BinanceAccount* ub = nullptr;
	BinanceAccount* cb = nullptr;
}accinfo;

class trader_Binance : public crypto_trader
{
public:
	trader_Binance(const std::string td_source_name, const int td_id);
	virtual ~trader_Binance();

public:
	//api
	virtual bool init(Json::Value conf)override;
	bool insert_order(const TBCryptoInputField* InputOrder)override;
	bool cancel_order(const TBCryptoInputField* ActionOrder)override;
	bool QueryOrder(Order& order);//交易时刻查询

	void cancel_all(std::string &id);

	bool QueryAccountInfo(AccountInfo& AI);
	bool QueryApiPosition(BinanceAccount* cb);
	//init,因为是通过curl调用拿到数据，这里不设置超时时间了
	bool QueryApiHisPosition();
	inline bool QryHisOrder();//初始化和post查询
	inline bool QryHisTrade();//初始化和post查询
	bool QryInitAccout();//查询第一个账户的可用资金

	bool QryOrderBySymbol(BinanceAccount* cb, std::string id, std::string symbol, long long& bid);
	bool QryTradeBySymbol(BinanceAccount* cb, std::string id, std::string symbol, long long& bid);
public:
	//spi
	void OnRtnOrder(Json::Value& json_result);
	void OnRtnTrade(Json::Value& json_result, Order *ord);
	void OnRtnOrderCash(Json::Value& json_result);
	void OnRtnTradeCash(Json::Value& json_result, Order *ord);
	void OnRspOrderInsert(const TBCryptoInputField* InputOrder, std::string& code, std::string &msg);
	void OnRspOrderAction(const TBCryptoInputField* ActionOrder, std::string& code, std::string& msg);

public:
	//tool func
	bool create_acc(std::string id, std::string& apikey, std::string seckey);
	bool getListenkey();
	void thread_start_websocket();
	void thread_keep_listen_key_alive();
	BinanceAccount* get_acc_from_symbol(std::string id, uint16_t crypto_type);

	std::string TBPriceType2BA(int ps, uint16_t crpto_type);
	int BAPriceType2TB(std::string &ps);

	std::string TBDirect2BA(int direct);
	int BADirect2TB(std::string& direct);

	int BAOrderStatus2TB(std::string&os);

	std::string TBTC2BA(int tc,int vc, const TBCryptoInputField* InputOrder);
	int BATC2TB(std::string & tc);

	void get_TOrder_from_Json(Json::Value& val, std::list<Order>& mlist, std::string& accid, long long &orderid);
	void get_Trade_from_Json(Json::Value& val, std::list<TBTradeData>& mlist, std::string& accid, long long& fromid);
	void parse_pos_from_json_array(Json::Value& val, bool is_cash);

	std::thread m_keep_thread;
	std::thread m_websocket_thread;
	bool m_PositionStatus = false;
	bool m_WebSocketInit = false;
	QueryType m_qtype;

	std::string m_AccID;

	std::unordered_map<std::string, accinfo> m_accout_map;
	std::vector<std::string> m_ubsymbol;
	std::vector<std::string> m_cbsymbol;
	std::vector<std::string> m_cash_symbol;
	std::unordered_map<std::string, int> m_sysid_2_oid;
	std::unordered_map<int, std::string> m_oid_2_sysid;
	std::unordered_map<std::string, int>m_BAOS2TB;
};
#endif