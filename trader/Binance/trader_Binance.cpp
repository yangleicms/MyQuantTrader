#include "trader_Binance.h"
#include "cell_log.h"
#include <boost/algorithm/string.hpp>

trader_Binance* callback;

int binance_order_event(Json::Value& json_result)
{
	std::string ordevent = json_result["e"].asString();
	if (ordevent == "executionReport")
	{
		callback->OnRtnOrderCash(json_result);
	}
	return 0;
}

int binance_order_event_fut(Json::Value& json_result)
{
	bool error = json_result.isMember("error");
	if (error)
	{
		SPDLOG_ERROR("error code:{},errormsg:{}",
			json_result["error"]["code"].asString().data(), json_result["error"]["msg"].asString().data());
		return -1;
	}

	std::string ordevent = json_result["e"].asString();
	if (ordevent == "ORDER_TRADE_UPDATE")
	{
		callback->OnRtnOrder(json_result);
	}
	else
	{
		int id = json_result["id"].asInt();
		if (id == BINANCE_CB_POSRSP_ID)
		{
			if (json_result.isMember("result"))
			{
				if (json_result["result"].size() == 0)
					return 0;
				Json::Value value = json_result["result"][0]["res"]["positions"];

				callback->parse_pos_from_json_array(value, false);
				callback->m_PositionStatus = true;
			}
		}
	}
	return 0;
}

trader_Binance::trader_Binance(const std::string td_source_name, const int td_id) :
	crypto_trader(td_source_name, td_id)
{
	callback = this;

	m_BAOS2TB["NEW"] = OrderStatus_NoTradeQueueing;
	m_BAOS2TB["PARTIALLY_FILLED"] = OrderStatus_PartTradedQueueing;
	m_BAOS2TB["FILLED"] = OrderStatus_AllTraded;
	m_BAOS2TB["CANCELED"] = OrderStatus_Canceled;
	m_BAOS2TB["EXPIRED"] = OrderStatus_NoTradeNotQueueing;
	m_BAOS2TB["REJECTED"] = OrderStatus_Error;

	m_exchange_id = EXCHANGE_BINANCE;
}

trader_Binance::~trader_Binance()
{
	for (auto it : m_accout_map)
	{
		if (it.second.cash != nullptr)
			delete it.second.cash;
		if (it.second.cb != nullptr)
			delete it.second.cb;
		if (it.second.ub != nullptr)
			delete it.second.ub;
	}

	m_accout_map.clear();
}

bool trader_Binance::create_acc(std::string id, std::string& apikey, std::string seckey)
{
	auto it = m_accout_map.find(id);
	if (it != m_accout_map.end())
		return false;
	accinfo info;
	info.cash = new BinanceAccount(binanceType::binance_cash, apikey, seckey);
	info.cb = new BinanceAccount(binanceType::binance_cb_fut, apikey, seckey);
	info.ub = new BinanceAccount(binanceType::binance_usdt_fut, apikey, seckey);

	m_accout_map[id] = info;
	return true;
}

bool trader_Binance::getListenkey()
{
	for (auto& it : m_accout_map)
	{
		SPDLOG_INFO("{} trader_Binance::getListenkey", it.first.data());
		Json::Value val;
		it.second.cash->startUserDataStream(val);
		std::string cash_key = val["listenKey"].asString();
		it.second.cash->m_listen_key = cash_key;

		it.second.ub->startUserDataStream(val);
		std::string ub_key = val["listenKey"].asString();
		it.second.ub->m_listen_key = ub_key;

		it.second.cb->startUserDataStream(val);
		std::string cb_key = val["listenKey"].asString();
		it.second.cb->m_listen_key = cb_key;
		std::cout << cb_key << std::endl;

		if (cash_key.size() * ub_key.size() * cb_key.size() == 0)
		{
			SPDLOG_ERROR("{} get listenkey error,please check", it.first.data());
			return false;
		}

		SPDLOG_INFO("trader_Binance::getListenkey,ub_key:{},cb_key:{}", ub_key.data(), cb_key.data());
	}
	return true;
}

void trader_Binance::thread_start_websocket()
{
	Webclient::curl_init();
	for (auto& it : m_accout_map)
	{
		std::string cash_stream = "/ws/" + it.second.cash->m_listen_key;
		std::string ub_stream = "/ws/" + it.second.ub->m_listen_key;
		std::string cb_stream = "/ws/" + it.second.cb->m_listen_key;

		it.second.cash->wscli = Webclient::connect_endpoint_cash(binance_order_event, cash_stream.data());
		it.second.ub->wscli = Webclient::connect_endpoint_ub(binance_order_event_fut, ub_stream.data());
		it.second.cb->wscli = Webclient::connect_endpoint_cb(binance_order_event_fut, cb_stream.data());

	}
	m_WebSocketInit = true;
	Webclient::work();
}

void trader_Binance::thread_keep_listen_key_alive()
{
	while (1)
	{
		std::this_thread::sleep_for(std::chrono::seconds(60));
		for (auto& it : m_accout_map)
		{
			//it.second.cash->keepUserDataStream(it.second.cash->m_listen_key.data());
			//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			it.second.ub->keepUserDataStream(it.second.ub->m_listen_key.data());
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			it.second.cb->keepUserDataStream(it.second.cb->m_listen_key.data());
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		}
	}
}

BinanceAccount* trader_Binance::get_acc_from_symbol(std::string id, uint16_t crypto_type)
{
	auto it = m_accout_map.find(id);
	if (it == m_accout_map.end())
	{
		return nullptr;
	}

	if (crypto_type == CRYPTO_USDTBASE_FUT_PERPETUAL || crypto_type == CRYPTO_USDTBASE_FUT_DELIVERY)
		return it->second.ub;
	if (crypto_type == CRYPTO_COINBASE_FUT_PERPETUAL || crypto_type == CRYPTO_COINBASE_FUT_DELIVERY)
		return it->second.cb;
	return it->second.cash;
}

bool trader_Binance::init(Json::Value conf)
{
	Webclient::curl_init();
	crypto_trader::init(conf);

	std::string AccID = conf["accout_id"].asString();
	m_AccID = AccID;
	std::string ApiKey = conf["api_key"].asString();
	std::string SecretKey = conf["secret_key"].asString();
	if (AccID.size()*ApiKey.size()*SecretKey.size() == 0) {
		std::cout << "binance init error:accout_id,api_key,secret_key should not be null\n";
		SPDLOG_ERROR("binance init error:accout_id,api_key,secret_key should not be null\n");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		exit(0);
	}
	SPDLOG_INFO("binance init,Accid:{},ApiKey:{},SecretKey:{}", AccID, ApiKey, SecretKey);

	std::vector<std::string> SymbolListUB, SymbolListCB, SymbolListCash;
	std::string UBString = conf["symbol_list_ub"].asString();
	std::string CBString = conf["symbol_list_cb"].asString();
	std::string CashString = conf["symbol_list_cash"].asString();

	boost::split(SymbolListUB, UBString, boost::is_any_of(","));
	boost::split(SymbolListCB, CBString, boost::is_any_of(","));
	boost::split(SymbolListCash, CashString, boost::is_any_of(","));

	create_acc(AccID, ApiKey, SecretKey);

	for (auto& it : SymbolListUB) {
		m_ubsymbol.push_back(it);
	}
	for (auto& it : SymbolListCB) {
		m_cbsymbol.push_back(it);
	}
	for (auto& it : SymbolListCash) {
		m_cbsymbol.push_back(it);
	}

	SPDLOG_INFO("Now GetlistenKey");
	getListenkey();
	std::thread t1(std::bind(&trader_Binance::thread_start_websocket, this));
	t1.swap(m_websocket_thread);
	std::thread t2(std::bind(&trader_Binance::thread_keep_listen_key_alive, this));
	t2.swap(m_keep_thread);

	int wait = 5;
	m_WebSocketInit = false;
	bool res = false;
	while (m_WebSocketInit == false && wait > 0)
	{
		--wait;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	if (m_WebSocketInit == false)
	{
		SPDLOG_ERROR("m_WebSocketInit is fail");
		return false;
	}
	SPDLOG_INFO("m_WebSocketInit Fin");
	//return true;
	//QryInitAccout();

	SPDLOG_INFO("Now Query Positon");
	res = QueryApiHisPosition();
	if (!res)
		return false;

	//QryHisOrder();
	//QryHisTrade();
	/*auto len = m_oms->m_cur_order_page.writer->ring_size;
	for(int i = 0; i <= len; ++i) {
		Order*ord;
		m_oms->get_order_from_id(i, ord);
		std::cout << i << "," << ord->key << std::endl;
	}*/
	return true;
}

bool trader_Binance::insert_order(const TBCryptoInputField* InputOrder)
{
	BinanceAccount* cb = get_acc_from_symbol(m_AccID, InputOrder->CryptoType);
	if (cb == nullptr) {
		SPDLOG_ERROR("trader_Binance::InputOrde,Can not find acc:{}", m_AccID);
		return false;
	}

	std::string code,msg;
	
	Json::Value res;
	std::string side = TBDirect2BA(InputOrder->Direction);
	std::string pt = TBPriceType2BA(InputOrder->PriceType, InputOrder->CryptoType);
	std::string tc = TBTC2BA(InputOrder->TimeCondition, InputOrder->VolumeCondition, InputOrder);
	double qty = InputOrder->VolumeTotal;

	double price = InputOrder->LimitPrice;
	std::string ordid = std::to_string(InputOrder->key);

	Order *ord;
	bool findit = m_oms->get_order_from_id(InputOrder->key, ord);
	if (false == findit) {
		SPDLOG_ERROR("trader_Binance::insert_order,can not find order in oms: {} {}!", InputOrder->key, InputOrder->InstrumentID);
		return false;
	}
	update_order(InputOrder, ord);

	//SPDLOG_DEBUG("trader_Binance::insert_order insert order:{},{}", InputOrder->key, ord->key);

	//底层发单通过libcurl的同步方法实现，因此我们需要把改写ord的操作放到下单之前;
	binanceError_t berror = cb->sendOrder(res, InputOrder->InstrumentID, side.data(), pt.data(), tc.data(), qty, price, ordid.data(), 0, 0, 3000);
	if (berror != binanceError_t::binanceSuccess)
	{
		SPDLOG_ERROR("trader_Binance::InputOrder fail:{}", binanceGetErrorString(berror));
		msg = binanceGetErrorString(berror);
		code = "-1";
		this->OnRspOrderInsert(InputOrder, code, msg);
		return false;
	}
	code = res["code"].asString();
	if (code.size() != 0)
	{
		msg = res["msg"].asString();
		this->OnRspOrderInsert(InputOrder, code, msg);
	}
	return true;
}

bool trader_Binance::cancel_order(const TBCryptoInputField* ActionOrder)
{
	std::string errID = "-1";
	std::string msg;
	BinanceAccount* cb = get_acc_from_symbol(m_AccID, ActionOrder->CryptoType);
	if (cb == nullptr)
	{
		SPDLOG_ERROR("trader_Binance::cancel_order,Can not find acc:{}", m_AccID);
		msg = "can not find acc";
		this->OnRspOrderAction(ActionOrder, errID, msg);
		return false;
	}
	Json::Value res;

	Order * order = nullptr;
	bool qres = m_oms->get_order_from_id(ActionOrder->key, order);

	if (false == qres){
		SPDLOG_ERROR("trader_Binance::cancel_order, key={}. Has no OrigOrder.", ActionOrder->key);
		msg = "can not find orderID "+ std::to_string(ActionOrder->key) + " in oms";
		this->OnRspOrderAction(ActionOrder, errID, msg);
		return false;
	}

	//SPDLOG_DEBUG("trader_Binance::cancel_order,cancel order:{},{}", ActionOrder->key, order->key);

	binanceError_t berror;
	if (order->OrderStatus != OrderStatus_UnKnown) {
		berror = cb->cancelOrder(res, ActionOrder->InstrumentID, order->OrderSysID, "", "", 3000);
	}
	else {
		SPDLOG_WARN("order kye:{} is unknow,try cancel with localID ", order->key);
		berror = cb->cancelOrder(res, ActionOrder->InstrumentID, "", std::to_string(ActionOrder->key).data(), "", 3000);
	}
	if (berror != binanceError_t::binanceSuccess) {
		SPDLOG_ERROR("trader_Binance::ActionOrder fail:{}", binanceGetErrorString(berror));
		return false;
	}

	std::string code = res["code"].asString();
	if (code.size() != 0)
	{
		std::string msg = res["msg"].asString();
		this->OnRspOrderAction(ActionOrder, code, msg);
	}
	return true;
}

bool trader_Binance::QueryOrder(Order& order)
{
	const char* sysid = order.OrderSysID;
	BinanceAccount* cb = get_acc_from_symbol(m_AccID, order.CryptoType);
	if (cb == nullptr) {
		SPDLOG_ERROR("trader_Binance::QueryOrder,Can not find acc:{}", m_AccID);
		return false;
	}
	Json::Value res;
	binanceError_t berror = cb->getOrder(res, order.InstrumentID, "", sysid, 3000);
	if (berror != binanceError_t::binanceSuccess) {
		SPDLOG_ERROR("trader_Binance::ActionOrder fail:{}", binanceGetErrorString(berror));
		return false;
	}

	std::string code;
	if (res["code"].isString())
		code = res["code"].asString();
	if (code.size() != 0)
	{
		std::string msg = res["msg"].asString();
		SPDLOG_ERROR("QueryOrder:errorID:{},errormsg:{} ", code.data(), msg.data());
		return false;
	}

	std::string baos = res["status"].asString();
	if (baos.size() != 0)
	{
		int os = BAOrderStatus2TB(baos);
		order.OrderStatus = os;
		std::string TradeV = res["executedQty"].asString();
		order.TradeVolume = atoi(TradeV.data());
	}
	else
	{
		order.OrderStatus = OrderStatus_UnKnown;
		order.TradeVolume = 0;
		return false;
	}

	return true;
}

bool trader_Binance::QryInitAccout()
{
	if (m_accout_map.size() == 0)
	{
		SPDLOG_WARN("trader_Binance::QryInitAccout,no accout info");
		return false;
	}
	auto acc = m_accout_map.begin();
	AccountInfo AI;
	memset(&AI, 0, sizeof(AI));
	strcpy(AI.ProductId, "USDT");
	strcpy(AI.InvestorID, acc->first.data());
	SPDLOG_INFO("trader_Binance::QryInitAccout,accout ID:{}", AI.InvestorID);
	return QueryAccountInfo(AI);
}

bool trader_Binance::QueryAccountInfo(AccountInfo& AI)
{
	if (m_accout_map.size() == 0)
	{
		SPDLOG_WARN("trader_Binance::QryInitAccout,no accout info");
		return false;
	}
	auto acc = m_accout_map.begin();
	memset(&AI, 0, sizeof(AI));
	strcpy(AI.InvestorID, acc->first.data());

	BinanceAccount* cb = get_acc_from_symbol(AI.InvestorID, CRYPTO_USDTBASE_FUT_PERPETUAL);
	if (cb == nullptr)
	{
		SPDLOG_ERROR("trader_Binance::QueryAccountInfo,Can not find acc:{}", AI.InvestorID);
		return false;
	}

	Json::Value res;
	binanceError_t berror = cb->getAccoutBalance(res, 3000);
	if (berror != binanceError_t::binanceSuccess)
	{
		SPDLOG_ERROR("trader_Binance::QueryAccountInfo fail:{}", binanceGetErrorString(berror));
		return false;
	}

	std::string code;
	if (res.isArray() == false)
	{
		code = res["code"].asString();
		if (code.size() != 0)
		{
			std::string msg = res["msg"].asString();
			SPDLOG_ERROR("QueryAccountInfo:errorID:{},errormsg:{} ", code.data(), msg.data());
			return false;
		}
	}
	else
	{
		int len = res.size();
		for (int i = 0; i < len; ++i)
		{
			std::string asset = res[i]["asset"].asString();
			if (asset == "USDT")
			{
				std::string availableBalance = res[i]["availableBalance"].asString();
				AI.Available = atof(availableBalance.data());
				std::string balance = res[i]["balance"].asString();
				AI.Total = atof(balance.data());
				SPDLOG_INFO("trader_Binance::QueryAccountInfo,asset:{},available:{}", asset.data(), AI.Available);
				break;
			}
		}
	}

	BinanceAccount* cb2 = get_acc_from_symbol(AI.InvestorID, CRYPTO_COINBASE_FUT_PERPETUAL);
	if (cb2 == nullptr)
	{
		SPDLOG_ERROR("trader_Binance::QueryAccountInfo,Can not find acc:{}", AI.InvestorID);
		return false;
	}

	Json::Value res2;
	binanceError_t berror2 = cb2->getAccoutBalance(res, 3000);
	return true;

}

void trader_Binance::parse_pos_from_json_array(Json::Value& val, bool is_cash)
{
	if (val.isArray() == false)
		return;
	int len = val.size();
	if (false == is_cash) {
		for (int i = 0; i < len; ++i) {
			std::string symbol = val[i]["symbol"].asString();
			std::string positionAmt = val[i]["positionAmt"].asString();
			std::string positionSide = val[i]["positionSide"].asString();
			double vol = atof(positionAmt.data());
			if(is_zero(vol))
				continue;

			auto pos = get_or_create_trader_pos(symbol.data());
			if (positionSide == "BOTH")
				pos->tot_net_pos = vol;
			else if (positionSide == "LONG") {
				pos->long_pos = vol;
				pos->tot_net_pos += vol;
			}
			else
			{
				pos->short_pos = vol;
				pos->tot_net_pos -= vol;
			}

			char* fp = strstr((char*)symbol.data(), "USDT");
			if (fp == NULL)
				pos->CryptoType = CRYPTO_COINBASE_FUT_PERPETUAL;
			else
				pos->CryptoType = CRYPTO_USDTBASE_FUT_PERPETUAL;

			//#ifdef DEBUG
			if(false == is_zero(vol))
				SPDLOG_INFO("OnRspQryPosition Ins={} Pos={} positionSide:{} ", symbol.data(), vol, positionSide);
		}
	}
	else
	{
		for (int i = 0; i < len; ++i) {
			std::string symbol = val[i]["asset"].asString();
			std::string full_symbol = symbol + "USDT";
			//std::string full_symbol = symbol;
			std::string free = val[i]["free"].asString();
			std::string locked = val[i]["locked"].asString();
			double free_vol = atof(free.data());
			double locked_vol = atof(locked.data());
			if(is_zero(free_vol)&&is_zero(locked_vol))
				continue;

			auto pos = get_or_create_trader_pos(symbol.data());
			strncpy(pos->instrument_id, full_symbol.data(), sizeof(pos->instrument_id));
			pos->CryptoType = CRYPTO_SPOT;
			pos->tot_net_pos = free_vol + locked_vol;
			pos->freeze = locked_vol;

			if (false == is_zero(free_vol)|| false == is_zero(locked_vol))
				SPDLOG_INFO("OnRspQryPosition Ins={},FullIns={} free={} freeze:{} ", symbol, full_symbol, free_vol, locked);
		}
	}

}

bool trader_Binance::QueryApiPosition(BinanceAccount* cb)
{
	Json::Value res;
	binanceError_t berror = cb->getInfo(res);
	if (berror != binanceError_t::binanceSuccess)
	{
		SPDLOG_ERROR("trader_Binance::QryTradeBySymbol fail:{}", binanceGetErrorString(berror));
		return false;
	}

	Json::Value val;
	if (cb->m_type != binanceType::binance_cash) {
		val = res["positions"];
		parse_pos_from_json_array(val, false);
	}
	else {
		val = res["balances"];
		parse_pos_from_json_array(val, true);
	}

	return true;
}

bool trader_Binance::QueryApiHisPosition()
{
	if (m_accout_map.size() == 0)
		return false;
	auto acc = m_accout_map.begin();
	auto cb = acc->second.ub;
	this->QueryApiPosition(cb);//qry usdtbase postion

	cb = acc->second.cash;
	this->QueryApiPosition(cb);//qry usdtbase postion

	cb = acc->second.cb;
	//std::string reqstr = "{\"method\":\"REQUEST\",\"params\":[\"KN2Ab9Az4FujqcBZ5P8nq9vGv6AsVkM8ND2kKc0r6aeUgFOHoI8r2No5Trn6FmxA@position\"],\"id\":12 }";
	std::string reqstr = "{\"method\": \"REQUEST\",\"params\":[\""
		+ cb->m_listen_key + "@position\"],\"id\":" + std::to_string(BINANCE_CB_POSRSP_ID) + "}";
	cb->send_lws_msg(reqstr.data(), reqstr.size());
	int wait_sec = 5;

	while (!m_PositionStatus && wait_sec > 0) {
		wait_sec--;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	if (!m_PositionStatus) {
		SPDLOG_ERROR("trader_Binance::QueryApiHisPosition TimeOut,please make sure websocket Server is running");
		return false;
	}

	return true;
}

std::string trader_Binance::TBPriceType2BA(int ps, uint16_t crpto_type)
{
	if (ps == TD_PriceType_AnyPrice)
		return std::string("MARKET");
	if (ps == TD_PriceType_LimitPrice)
		return std::string("LIMIT");
	if (ps == TD_PriceType_LimitMarker) {
		if (crpto_type == CRYPTO_SPOT)
			return std::string("LIMIT_MAKER");
		else
			return std::string("LIMIT");
	}

}

int trader_Binance::BAPriceType2TB(std::string& ps)
{
	int res = (ps[0] == 'M') ? TD_PriceType_AnyPrice : TD_PriceType_LimitPrice;
	return res;
}

std::string trader_Binance::TBDirect2BA(int direct)
{
	if (direct == 0)
		return std::string("BUY");
	return std::string("SELL");
}

int trader_Binance::BADirect2TB(std::string& direct)
{
	int res = (direct[0] == 'B') ? 0 : 1;
	return res;
}

int trader_Binance::BAOrderStatus2TB(std::string& os)
{
	auto it = m_BAOS2TB.find(os);
	if (it == m_BAOS2TB.end())
		return OrderStatus_UnKnown;
	return it->second;
}

std::string trader_Binance::TBTC2BA(int tc, int vc, const TBCryptoInputField* InputOrder)
{
	if (InputOrder->CryptoType != CRYPTO_SPOT && InputOrder->PriceType == TD_PriceType_LimitMarker)
		return std::string("GTX");
	std::string res = "GTC";
	if (tc == TD_TimeCondition_IOC)
	{
		if (vc == TD_VolumeCondition_CV)
			res = "FOK";
		else
			res = "IOC";
	}
	return res;
}

int trader_Binance::BATC2TB(std::string& tc)
{
	if (tc == "GTC")
		return TD_TimeCondition_GFD;
	if (tc == "FOK" || tc == "IOC")
		return TD_TimeCondition_IOC;
	if (tc == "GTX")
		return TD_TimeCondition_GTX;
	return TD_TimeCondition_GFD;
}

void trader_Binance::OnRspOrderInsert(const TBCryptoInputField* InputOrder, std::string& code, std::string& msg)
{
	int64_t localOrderRef = InputOrder->key;
	if (localOrderRef < 0)
	{
		SPDLOG_ERROR("recv unknown request's input order response {}", localOrderRef);
		return;
	}

	TBEvent event;
	event.eventType = TBRspOrderInsert;
	event.pubSubKey = InputOrder->pubSubKey;
	event.key = localOrderRef;
	event.OrderStatus = OrderStatus_Error;
	event.Direction = InputOrder->Direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, InputOrder->InstrumentID);
	event.ErrorID = atoi(code.data());
	strncpy(event.errmsg, msg.data(),sizeof(event.errmsg)-1);

	event.InstrumentHashVal = InputOrder->TradeItemHashVal;
	event.StrategyID = InputOrder->StrategyID;
	event.SourceStrategyID = InputOrder->SourceStrategyID;
	

	SPDLOG_ERROR("OnRspOrderInsert:errorID:{},errormsg:{},key:{},instru:{},type:{},Direct:{} ",
		code.data(), msg.data(), InputOrder->key, InputOrder->InstrumentID,InputOrder->CryptoType,InputOrder->Direction);

	Order *ord = nullptr;
	bool res = m_oms->get_order_from_id(localOrderRef, ord);
	if (res) {
		if (ord->OrderStatus != OrderStatus_UnKnown) {
			SPDLOG_ERROR("trader_Binance::OnRspOrderInsert,order {} status:{} is not OrderStatus_UnKnown,this rsp maybe error", 
				ord->key, ord->OrderStatus);
			//return;
		}

		event.OrdPtr = ord;
		ord->OrderStatus = OrderStatus_Error;
	}
	else {
		SPDLOG_ERROR("trader_Binance::OnRspOrderInsert Can not find {} in oms", InputOrder->key);
		return;
	}
	m_rbwriter->push(event);
}

void trader_Binance::OnRspOrderAction(const TBCryptoInputField* ActionOrder, std::string& code, std::string& msg)
{
	int64_t localOrderRef = ActionOrder->key;
	if (localOrderRef < 0)
	{
		SPDLOG_ERROR("recv unknown request's input order response {}", localOrderRef);
		return;
	}

	TBEvent event;
	event.eventType = TBRspOrderAction;
	event.key = localOrderRef;
	event.pubSubKey = ActionOrder->pubSubKey;
	event.OrderStatus = OrderStatus_Canceling;
	event.ErrorID = atoi(code.data());
	event.Direction = ActionOrder->Direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ActionOrder->InstrumentID);
	strncpy(event.errmsg, msg.data(), sizeof(event.errmsg) - 1);

	event.InstrumentHashVal = ActionOrder->TradeItemHashVal;
	event.StrategyID = ActionOrder->StrategyID;
	event.SourceStrategyID = ActionOrder->SourceStrategyID;

	Order *ord = nullptr;
	bool res = m_oms->get_order_from_id(localOrderRef, ord);
	if (res)
		event.OrdPtr = ord;
	else
		SPDLOG_ERROR("trader_Binance::OnRspOrderAction Can not find {} in oms", localOrderRef);
	m_rbwriter->push(event);

	SPDLOG_ERROR("OnRspOrderAction:errorID:{},errormsg:{},key:{},instru:{} ", 
		code.data(), msg.data(), ActionOrder->key, ActionOrder->InstrumentID);

}

void trader_Binance::OnRtnOrder(Json::Value& json_result)
{
	std::string ordSysId = json_result["o"]["i"].asString();
	std::string baos = json_result["o"]["X"].asString();
	int os = BAOrderStatus2TB(baos);
	std::string ordKey = json_result["o"]["c"].asString();
	std::string totalOrdTraded = json_result["o"]["z"].asString();
	std::string instr = json_result["o"]["s"].asString();
	std::string price = json_result["o"]["p"].asString();
	std::string qty = json_result["o"]["q"].asString();

	std::string tradeVol = json_result["o"]["l"].asString();
	std::string tradePrice = json_result["o"]["L"].asString();
	double turnover = atof(tradePrice.data()) * atof(tradePrice.data());
	
	bool is_outer_ord = false;
	int64_t key = -1;
	Order *ord = nullptr;

	if (ordKey.size() == 0 || ordKey[0] > '9' || ordKey[0] < '0') {//APP手动单
		is_outer_ord = true;
	}
	else{
		key = atoll(ordKey.data());
	}

	if (is_outer_ord){
		SPDLOG_INFO("outSide Order,OnRtnOrder:key:{},instr:{},price:{},vol:{},trdVol:{}",
			ordKey.data(), instr.data(), price.data(), qty.data(), totalOrdTraded.data());
	}
	else{
		m_oms->get_order_from_id(key, ord);
		if (ord == nullptr) {
			SPDLOG_ERROR("trader_Binance::OnRtnOrder Can not find {} in oms", key);
			return;
		}

		TBEvent event;
		event.key = key;
		event.pubSubKey = ord->pubSubKey;
		event.eventType = TBRtnOrder;
		event.TotalTradeVolume = atof(totalOrdTraded.data());
		event.OrderStatus = os;
		event.StrategyID = ord->StrategyID;
		event.SourceStrategyID = ord->SourceStrategyID;
		event.InstrumentHashVal = ord->hash_key;
		event.OrdPtr = ord;
		event.Direction = ord->direction;
		event.ExchangeIdx = EXCHANGE_BINANCE;
		strcpy(event.InstrumentID, ord->InstrumentID);

		strcpy(ord->OrderSysID, ordSysId.c_str());
		if (false == is_final_status(ord->OrderStatus))
		{
			ord->OrderStatus = os;
			ord->TradeVolume = event.TotalTradeVolume;
			//SPDLOG_INFO("trader_Binance::OnRtnOrder ord {},os:{},trd:{}",ord->key,ord->OrderStatus,ord->TradeVolume);
			ord->TotalTurnOver += turnover;
		}
		event.TotalTurnOver = ord->TotalTurnOver;

		m_rbwriter->push(event);
	}

	std::string eventType = json_result["o"]["x"].asString();
	SPDLOG_INFO("OnRtnOrder:key:{},instr:{},price:{},vol:{},eventType:{},OS:{}", 
		key, instr.data(), price.data(), qty.data(), eventType.data(),baos);

	if (eventType == "TRADE")
	{
		{
			std::string tradeVol = json_result["o"]["l"].asString();
			std::string dir = json_result["o"]["S"].asString();
			int Direction = TD_LONG_DIRECTION;
			if ("SELL"== dir)
				Direction = TD_SHORT_DIRECTION;
			update_trade(instr.data(), Direction, atof(tradeVol.data()));
		}
		if (false == is_outer_ord) {
			OnRtnTrade(json_result, ord);
		}
	}
}

void trader_Binance::OnRtnTrade(Json::Value& json_result, Order *ord)
{
	std::string tradeVol = json_result["o"]["l"].asString();
	std::string tradePrice = json_result["o"]["L"].asString();
	std::string ordKey = json_result["o"]["c"].asString();
	std::string exeID = json_result["o"]["t"].asString();
	std::string ordSysId = json_result["o"]["i"].asString();

	TBEvent event;
	event.eventType = TBRtnTradeInfo;
	event.pubSubKey = ord->pubSubKey;
	event.LastTradeVolume = atof(tradeVol.data());
	event.TradePrice = atof(tradePrice.data());
	strcpy(event.TradeID, exeID.data());
	event.key = ord->key;
	event.InstrumentHashVal = ord->hash_key;
	event.StrategyID = ord->StrategyID;
	event.SourceStrategyID = ord->SourceStrategyID;
	event.OrdPtr = ord;
	event.Direction = ord->direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ord->InstrumentID);
	event.TotalTurnOver = ord->TotalTurnOver;
	event.TotalTradeVolume = ord->TradeVolume;

	std::string instr = json_result["o"]["s"].asString();
	if (0 != strcmp(instr.data(), ord->InstrumentID)) {
		SPDLOG_ERROR("instrument is not same:{},{}", instr, ord->InstrumentID);
	}

	m_rbwriter->push(event);

	SPDLOG_INFO("OnRtnTrade:key:{},instr:{},price:{},tradevol:{},TradeID:{}",
		event.key, json_result["o"]["s"].asString().data(), event.TradePrice, event.LastTradeVolume, event.TradeID);

}

void trader_Binance::OnRtnOrderCash(Json::Value& json_result)
{
	std::string ordSysId = json_result["i"].asString();
	std::string baos = json_result["X"].asString();
	int os = BAOrderStatus2TB(baos);
	std::string ordKey = json_result["C"].asString();
	std::string ordKey2 = json_result["c"].asString();
	std::string totalOrdTraded = json_result["z"].asString();
	std::string instr = json_result["s"].asString();
	std::string price = json_result["p"].asString();
	std::string qty = json_result["q"].asString();
	std::string TotalTurnOver = json_result["Z"].asString();

	bool is_out_ord = false;
	int64_t key = -1;
	Order *ord = nullptr;

	if (ordKey.size() == 0 || ordKey[0] > '9' || ordKey[0] < '0'){
		if (ordKey2.size() == 0 || ordKey2[0] > '9' || ordKey2[0] < '0') {
			is_out_ord = true;
		}
		else{
			key = atoll(ordKey2.data());
		}
	}
	else{
		key = atoll(ordKey.data());
	}

	std::string eventType = json_result["x"].asString();
	SPDLOG_INFO("OnRtnOrder:key:{},instr:{},price:{},vol:{},eventType:{},C:{},i:{},OS:{},os:{}", 
		key, instr.data(), price.data(), qty.data(), eventType.data(), json_result["C"].asString(), json_result["i"].asInt64(),baos,os);

	if (is_out_ord)
	{
		SPDLOG_INFO("outSide Order,OnRtnOrder,c:{},C:{},instr:{},price:{},vol:{},trdVol:{}",
			json_result["c"].asString(),json_result["C"].asString(), instr.data(), price.data(), qty.data(), totalOrdTraded.data());
	}
	else{
		m_oms->get_order_from_id(key, ord);
		if (ord == nullptr) {
			SPDLOG_ERROR("trader_Binance::OnRtnOrderCash,can not find ord:{}", key);
			return;
		}

		TBEvent event;
		event.key = key;
		event.eventType = TBRtnOrder;
		event.pubSubKey = ord->pubSubKey;
		event.TotalTradeVolume = atof(totalOrdTraded.data());
		event.TotalTurnOver = atof(TotalTurnOver.data());
		event.OrderStatus = os;
		event.StrategyID = ord->StrategyID;
		event.SourceStrategyID = ord->SourceStrategyID;
		event.InstrumentHashVal = ord->hash_key;
		event.OrdPtr = ord;
		event.Direction = ord->direction;
		event.ExchangeIdx = EXCHANGE_BINANCE;
		strcpy(event.InstrumentID, ord->InstrumentID);

		strcpy(ord->OrderSysID, ordSysId.c_str());
		if (false == is_final_status(ord->OrderStatus))
		{
			ord->OrderStatus = os;
			ord->TradeVolume = event.TotalTradeVolume;
			ord->TotalTurnOver = event.TotalTurnOver;
			//SPDLOG_INFO("trader_Binance::OnRtnOrderCash ord {},os:{},trd:{}", ord->key, ord->OrderStatus, ord->TradeVolume);
		}

		m_rbwriter->push(event);
	}

	if (eventType == "TRADE")
	{
		//else 
		{
			std::string tradeVol = json_result["l"].asString();
			std::string dir = json_result["S"].asString();
			int Direction = TD_LONG_DIRECTION;
			if ("BUY" != dir)
				Direction = TD_SHORT_DIRECTION;

			char *p = strstr((char*)instr.data(), "USD");
			if (p == nullptr){
				SPDLOG_ERROR("error:can not find USD in instrument:{},is it a SPOT instrumen?",instr);
				return;
			}
			int offset = p - instr.data();
			std::string sub = instr.substr(0, offset);
			update_trade(sub.data(), Direction, atof(tradeVol.data()));
		}
		if (false == is_out_ord) {
			OnRtnTradeCash(json_result, ord);
		}
	}

}

void trader_Binance::OnRtnTradeCash(Json::Value& json_result, Order *ord)
{
	std::string tradeVol = json_result["l"].asString();
	std::string tradePrice = json_result["L"].asString();
	std::string exeID = json_result["t"].asString();
	std::string ordSysId = json_result["i"].asString();
	std::string totalOrdTraded = json_result["z"].asString();
	std::string TotalTurnOver = json_result["Z"].asString();

	TBEvent event;
	event.eventType = TBRtnTradeInfo;
	event.LastTradeVolume = atof(tradeVol.data());
	event.TradePrice = atof(tradePrice.data());
	event.TotalTradeVolume = atof(totalOrdTraded.data());
	event.TotalTurnOver = atof(TotalTurnOver.data());
	strcpy(event.TradeID, exeID.data());
	event.key = ord->key;
	event.InstrumentHashVal = ord->hash_key;
	event.StrategyID = ord->StrategyID;
	event.SourceStrategyID = ord->SourceStrategyID;
	event.OrdPtr = ord;
	event.Direction = ord->direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ord->InstrumentID);

	m_rbwriter->push(event);

	SPDLOG_INFO("OnRtnTrade:key:{},instr:{},price:{},tradevol:{},TradeID:{}",
		event.key, event.InstrumentID, event.TradePrice, event.LastTradeVolume, event.TradeID);

}

void trader_Binance::get_TOrder_from_Json(Json::Value& val, std::list<Order>& mlist, std::string &id, long long &orderld)
{
	if (val.isArray())
	{
		int len = val.size();
		for (int i = 0; i < len; ++i)
		{
			Order order;
			memset(&order, 0, sizeof(order));
			std::string clientOrderId = val[i]["clientOrderId"].asString();
			int64_t key = 0;
			bool is_out_ord = false;
			if (clientOrderId.size() == 0 || clientOrderId[0] > '9' || clientOrderId[0] < '0')//APP手动单，忽略
			{
				SPDLOG_WARN("order key is {},it should be a app order,igonre", clientOrderId.data());
				is_out_ord = true;
			}
			else
			{
				key = atoi(clientOrderId.data());
			}
			order.key = key;
			strcpy(order.InstrumentID, val[i]["symbol"].asString().data());

			long long sysid = val[i]["orderId"].asInt64();
			if (sysid > orderld)
				orderld = sysid;
			strcpy(order.OrderSysID, std::to_string(sysid).data());

			std::string side = val[i]["side"].asString();
			order.direction = BADirect2TB(side);
			std::string tc = val[i]["timeInForce"].asString();
			order.time_condition = BATC2TB(tc);
			std::string pt = val[i]["type"].asString();
			order.price_type = BAPriceType2TB(pt);
			std::string price = val[i]["price"].asString();
			order.LimitPrice = atof(price.data());
			std::string origQty = val[i]["origQty"].asString();

			order.VolumeTotal = atof(origQty.data());
			std::string status = val[i]["status"].asString();
			order.OrderStatus = BAOrderStatus2TB(status);
			std::string TradeV = val[i]["executedQty"].asString();
			order.TradeVolume = atof(TradeV.data());

			m_sysid_2_oid[order.OrderSysID] = order.key;
			m_oid_2_sysid[order.key] = order.OrderSysID;
			mlist.push_back(order);
		}
	}
}

void trader_Binance::get_Trade_from_Json(Json::Value& val, std::list<TBTradeData>& mlist, std::string& accid, long long& fromid)
{
	if (val.isArray())
	{
		int len = val.size();
		for (int i = 0; i < len; ++i)
		{
			TBTradeData data;
			memset(&data, 0, sizeof(data));
			strcpy(data.InstrumentID, val[i]["symbol"].asString().data());

			long long tid = val[i]["id"].asInt64();
			strcpy(data.TradeID, std::to_string(tid).data());
			std::string sysid = val[i]["orderId"].asString();
			strcpy(data.OrderSysID, sysid.data());

			auto it = m_sysid_2_oid.find(data.OrderSysID);
			if (it != m_sysid_2_oid.end())
				data.key = it->second;
			else
			{
				SPDLOG_WARN("order sysid is {} and cannot find in hisOrder,it should be a trade from app order,igonre", sysid.data());
				continue;
			}

			std::string side = val[i]["side"].asString();
			data.Direction = BADirect2TB(side);
			std::string price = val[i]["price"].asString();
			data.TradePrice = atof(price.data());
			std::string qty = val[i]["qty"].asString();
			data.TradeVolume = atof(qty.data());
			mlist.push_back(data);
		}
	}
}

bool trader_Binance::QryOrderBySymbol(BinanceAccount*cb, std::string id, std::string symbol, long long &bid)
{
	long long nowtp, begin;
	int get_num_per_http = 50;
	int ordernumub = 0;
	SPDLOG_INFO("{}:query {} UBhisOrder,begin id:%lld", id.data(), symbol.data(), bid);

	Json::Value res;
	get_ms_epoch(nowtp, begin);
	binanceError_t berror = cb->getAllOrders(res, symbol.data(), bid, begin, nowtp, get_num_per_http, 3000);
	if (berror != binanceError_t::binanceSuccess)
	{
		SPDLOG_ERROR("trader_Binance::QryOrderBySymbol fail:{}", binanceGetErrorString(berror));
		return false;
	}

	std::string code;
	if (res.isArray() == false)
	{
		code = res["code"].asString();
		if (code.size() != 0)
		{
			std::string msg = res["msg"].asString();
			SPDLOG_ERROR("trader_Binance QryOrderBySymbol:errorID:{},errormsg:{} ", code.data(), msg.data());
			false;
		}
	}

	std::list<Order> mlist;
	get_TOrder_from_Json(res, mlist, id, bid);

	for (auto it : mlist)
	{
		SPDLOG_INFO("Init Order,key:{},instr:{},price:{},OrderSYSID:{},Vol:{},tradeVol:{}",
			it.key, it.InstrumentID, it.LimitPrice, it.OrderSysID, it.VolumeTotal, it.TradeVolume);
	}

	if (res.size() < get_num_per_http)
		return false;

	++bid;//此处自增1避免收到重复的order
	return true;
}

bool trader_Binance::QryTradeBySymbol(BinanceAccount* cb, std::string id, std::string symbol, long long& bid)
{
	long long nowtp, begin;
	int get_num_per_http = 50;
	int ordernumub = 0;
	SPDLOG_INFO("{}:query {} UBhisTrade,begin id:%lld", id.data(), symbol.data(), bid);

	Json::Value res;
	get_ms_epoch(nowtp, begin);
	binanceError_t berror = cb->getAllTrades(res, symbol.data(), bid, begin, nowtp, get_num_per_http, 3000);
	if (berror != binanceError_t::binanceSuccess)
	{
		SPDLOG_ERROR("trader_Binance::QryTradeBySymbol fail:{}", binanceGetErrorString(berror));
		return false;
	}

	std::string code;
	if (res.isArray() == false)
	{
		code = res["code"].asString();
		if (code.size() != 0)
		{
			std::string msg = res["msg"].asString();
			SPDLOG_ERROR("trader_Binance QryTradeBySymbol:errorID:{},errormsg:{} ", code.data(), msg.data());
			false;
		}
	}

	std::list<TBTradeData> mlist;
	get_Trade_from_Json(res, mlist, id, bid);


	for (auto it : mlist) {
		SPDLOG_INFO("Init Trade,TID:%lld,instr:{},Tradeprice:{},OrderSYSID:{},tradeVol:{}",
			it.TradeID, it.InstrumentID, it.TradePrice, it.OrderSysID, it.TradeVolume);
	}

	if (res.size() < get_num_per_http)
		return false;

	++bid;//此处自增1避免收到重复的order
	return true;
}

bool trader_Binance::QryHisOrder()
{
	for (auto& it : m_accout_map)
	{
		for (auto& itr : m_ubsymbol)
		{
			auto cb = it.second.ub;
			long long orderidub = 0;
			while (1)
			{
				bool res = QryOrderBySymbol(cb, it.first, itr, orderidub);
				if (res == false)
					break;
			}
		}

		for (auto& itr : m_cbsymbol)
		{
			auto cb = it.second.cb;
			long long orderidcb = 0;
			while (1)
			{
				bool res = QryOrderBySymbol(cb, it.first, itr, orderidcb);
				if (res == false)
					break;
			}
		}
	}

	return true;
}

bool trader_Binance::QryHisTrade()
{
	for (auto& it : m_accout_map)
	{
		for (auto& itr : m_ubsymbol)
		{
			auto cb = it.second.ub;
			long long orderidub = 0;
			while (1)
			{
				bool res = QryTradeBySymbol(cb, it.first, itr, orderidub);
				if (res == false)
					break;
			}
		}

		for (auto& itr : m_cbsymbol)
		{
			auto cb = it.second.cb;
			long long orderidcb = 0;
			while (1)
			{
				bool res = QryTradeBySymbol(cb, it.first, itr, orderidcb);
				if (res == false)
					break;
			}
		}
	}

	return true;
}

void trader_Binance::cancel_all(std::string& id)
{
	auto it = m_accout_map.find(id);
	if (it == m_accout_map.end())
	{
		return;
	}

	auto cb = it->second.cb;
	auto ub = it->second.ub;

	for (auto &it : m_cbsymbol)
	{
		Json::Value res;
		cb->cancelAllOrder(res, it.data());
	}
	for (auto &it : m_ubsymbol)
	{
		Json::Value res;
		cb->cancelAllOrder(res, it.data());
	}
	for (auto &it : m_cash_symbol)
	{
		Json::Value res;
		cb->cancelAllOrder(res, it.data());
	}
}

