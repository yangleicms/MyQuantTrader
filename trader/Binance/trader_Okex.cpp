#include "trader_Okex.h"
#include "cell_log.h"
#include <boost/algorithm/string.hpp>

const int okex_ws_sleep_time = 12;
trader_Okex* okex_callback;

int okex_rtn_event(Json::Value& jr)
{
	if (false == jr["event"].empty()) {
		std::string event = jr["event"].asString();
		std::string code = jr["code"].asString();
		std::string msg = jr["msg"].asString();
		if (event == "login") {
			if (code!= "0") {
				SPDLOG_ERROR("Okex Login WebSocket Fail,msg:{}",msg);
				std::cout << "Okex Login WebSocket Fail:" << msg << std::endl;
				cell_sleep_us(1000 * 1000);
				exit(0);
			}
		}
		else {
			std::string strr = jr.toStyledString();
			SPDLOG_ERROR("{}", strr);
		}
		return 0;
	}

	if (false == jr["op"].empty()) 
	{
		std::string code = jr["code"].asString();
		if (code != "0") 
		{
			std::string msg = jr["msg"].asString();
			int64_t localID = atol(jr["data"][0]["clOrdId"].asString().data());
			Order* ord = nullptr;
			bool res = okex_callback->m_oms->get_order_from_id(localID, ord);
			if (false == res) {
				SPDLOG_ERROR("okex_rtn_event can not find ord:{}", localID);
				return -1;
			}

			if ("order" == jr["op"].asString()) {
				okex_callback->OnRspOrderInsert(ord, code, msg);
			}
			else if ("cancel-order" == jr["op"].asString()) {
				okex_callback->OnRspOrderAction(ord, code, msg);
			}
		}
		else {
			std::string strr = jr.toStyledString();
			SPDLOG_ERROR("{}", strr);
		}
		return 0;
	}
	
	auto arg = jr["arg"];
	if (true == arg.empty()) {
		std::string strr = jr.toStyledString();
		SPDLOG_ERROR("{}", strr);
		return 0;
	}

	std::string channel = arg["channel"].asString();
	if (channel == "orders") {
		auto dat = jr["data"];
		int len = dat.size();
		for (int i = 0; i < len; ++i) {
			okex_callback->OnRtnOrder(dat[i]);
		}
		return 0;
	}

	std::string strr = jr.toStyledString();
	SPDLOG_ERROR("{}", strr);
	return 0;
	
}

trader_Okex::trader_Okex(const std::string td_source_name, const int td_id) :
	crypto_trader(td_source_name, td_id)
{
	okex_callback = this;

	m_OKOS2TB["live"] = OrderStatus_NoTradeQueueing;
	m_OKOS2TB["partially_filled"] = OrderStatus_PartTradedQueueing;
	m_OKOS2TB["filled"] = OrderStatus_AllTraded;
	m_OKOS2TB["canceled"] = OrderStatus_Canceled;

	m_exchange_id = EXCHANGE_OKEX;
}

void trader_Okex::thread_start_websocket()
{
	m_acc->wscli = Webclient::connect(okex_rtn_event, OKEX_WS_PRI_PATH, std::to_string(okex_port), 
		OKEX_WS_HOST,std::bind(&trader_Okex::OnWsReconn,this));
	m_WebSocketInit = true;
	Webclient::work();
}

void trader_Okex::thread_keep_alive()
{
	while (1)
	{
		std::this_thread::sleep_for(std::chrono::seconds(okex_ws_sleep_time));
		auto cli = m_acc->wscli;
		cli->Send("ping");
	}
}

void trader_Okex::query_all_pos()
{
	Json::Value jv;

	m_acc->get_pos(jv);

	std::string code = jv["code"].asString();
	if (code != "0") {
		std::string msg = jv["msg"].asString();
		SPDLOG_ERROR("okex query pos fail:{},{}", code, msg);
	}
	else
	{
		auto dat = jv["data"];
		int len = dat.size();
		for (int i = 0; i < len; ++i) {
			std::string symbol = dat[i]["instId"].asString();
			auto pos = get_or_create_trader_pos(symbol.data());
			std::string posSide = dat[i]["posSide"].asString();
			double vol = atof(dat[i]["pos"].asString().data());

			if (posSide == "long") {
				pos->tot_net_pos = vol;
				pos->long_pos = vol;
			}
			else if (posSide == "short") {
				pos->tot_net_pos = -vol;
				pos->short_pos = vol;
			}
			else {
				pos->tot_net_pos = vol;
			}

			SPDLOG_INFO("OnRspQryPosition Ins={},positon:{} ", symbol, vol);
		}
	}

	jv.clear();
	m_acc->get_spot_pos(jv);
	code = jv["code"].asString();
	if (code != "0") {
		std::string msg = jv["msg"].asString();
		SPDLOG_ERROR("okex query spot pos fail:{},{}", code, msg);
	}
	else
	{
		auto dat = jv["data"][0]["details"];
		int len = dat.size();
		for (int i = 0; i < len; ++i) {
			std::string symbol = dat[i]["ccy"].asString();
			auto pos = get_or_create_trader_pos(symbol.data());
			double vol = atof(dat[i]["eq"].asString().data());
			pos->tot_net_pos = vol;
			SPDLOG_INFO("OnRspQryPosition spot_Ins={},positon:{} ", symbol, vol);
		}
		
	}
}

void trader_Okex::login_ws()
{
	std::string tp = std::to_string(get_current_ms_epoch() / 1000);
	std::string strr = tp + "GET/users/self/verify";

	Json::Value jsObj, sub, args;
	sub["apiKey"] = m_apiKey.data();
	sub["passphrase"] = m_passwd.data();
	sub["timestamp"] = tp;
	sub["sign"] = get_okex_sign(m_secKey.data(), strr.data());
	args.append(sub);

	jsObj["op"] = "login";
	jsObj["args"] = args;
	std::string jsonstr = jsObj.toStyledString();
	//std::cout << jsonstr << std::endl;
	m_acc->wscli->Send(jsonstr);
}

void trader_Okex::sub_order_channel(std::string strr)
{
	std::vector<std::string> vec;
	boost::split(vec, strr, boost::is_any_of(","));

	if (vec.size() < 1) {
		SPDLOG_ERROR("Okex sub_code fail,codestrr:{}", strr);
		std::cout << "Okex sub_code WebSocket Fail:" << strr << std::endl;
		cell_sleep_us(1000 * 1000);
		exit(0);
	}

	Json::Value jsObj, sub, args;

	jsObj["op"] = "subscribe";
	for (auto& it : vec) {
		sub.clear();
		sub["channel"] = "orders";
		
		auto findit = it.find("SWAP");
		if(findit!=std::string::npos)
			sub["instType"] = "SWAP";
		else
			sub["instType"] = "SPOT";

		sub["instId"] = it;
		args.append(sub);
	}
	jsObj["args"] = args;
	m_ws_subKey = jsObj.toStyledString();
	m_acc->wscli->Send(m_ws_subKey);
}

void trader_Okex::OnWsReconn()
{
	cell_sleep_us(100 * 1000);
	SPDLOG_WARN("trader_Okex::OnWsReconn,reLogin ws srv");
	login_ws();
	cell_sleep_us(100 * 1000);
	SPDLOG_WARN("trader_Okex::OnWsReconn,resub order_channel:{}", m_ws_subKey);
	m_acc->wscli->Send(m_ws_subKey);
}

bool trader_Okex::init(Json::Value conf)
{
	crypto_trader::init(conf);
	m_apiKey = conf["api_key"].asString();
	m_secKey = conf["sec_key"].asString();
	m_passwd = conf["passwd"].asString();
	std::string sub_code = conf["sub_code"].asString();

	SPDLOG_INFO("trader_Okex init,apiKey:{},secKey:{},passwd:{}", m_apiKey, m_secKey, m_passwd);

	m_acc = new OkexAccount(m_apiKey, m_secKey, m_passwd);
	std::thread t1(std::bind(&trader_Okex::thread_start_websocket, this));
	t1.swap(m_websocket_thread);

	std::thread t2(std::bind(&trader_Okex::thread_keep_alive, this));
	t2.swap(m_keep_thread);
	SPDLOG_INFO("trader_Okex init WebSockets Fin");

	cell_sleep_us(1000 * 1000);
	SPDLOG_INFO("trader_Okex start Login WebSockets");
	login_ws();

	cell_sleep_us(1000 * 1000);
	SPDLOG_INFO("trader_Okex start sub order channel");
	sub_order_channel(sub_code);


	SPDLOG_INFO("trader_Okex start qry position");
	query_all_pos();
	SPDLOG_INFO("trader_Okex init Fin");
	return true;
}

bool trader_Okex::insert_order(const TBCryptoInputField* InputOrder)
{
	std::string code, msg;
	Json::Value jr;

	Order* ord;
	bool findit = m_oms->get_order_from_id(InputOrder->key, ord);
	if (false == findit) {
		SPDLOG_ERROR("trader_Binance::insert_order,can not find order in oms: {} {}!", InputOrder->key, InputOrder->InstrumentID);
		return false;
	}
	update_order(InputOrder, ord);

	m_acc->send_order(InputOrder, jr);

	code = jr["code"].asString();
	if (code != "0") {
		msg = jr["data"][0]["sMsg"].asString();
		OnRspOrderInsert(InputOrder, code, msg);
	}

	return true;
}

bool trader_Okex::cancel_order(const TBCryptoInputField* ActionOrder)
{
	std::string code, msg;

	Order* order = nullptr;
	bool qres = m_oms->get_order_from_id(ActionOrder->key, order);

	if (false == qres) {
		SPDLOG_ERROR("trader_Binance::cancel_order, key={}. Has no OrigOrder.", ActionOrder->key);
		msg = "can not find orderID " + std::to_string(ActionOrder->key) + " in oms";
		this->OnRspOrderAction(ActionOrder, code, msg);
		return false;
	}

	Json::Value jr;
	m_acc->cancel_order(ActionOrder->InstrumentID,order->OrderSysID, ActionOrder->key,jr);
	code = jr["code"].asString();
	if (code != "0") {
		msg = jr["data"][0]["sMsg"].asString();
		OnRspOrderAction(ActionOrder, code, msg);
	}

	return true;
}

int trader_Okex::OrderStatus2TD(std::string& os)
{
	auto it = m_OKOS2TB.find(os);
	if (it == m_OKOS2TB.end())
		return OrderStatus_UnKnown;
	return it->second;
}

bool trader_Okex::get_order(Order& order)
{
	return true;
}

void trader_Okex::OnRtnOrder(Json::Value& json_result)
{
	std::string ordSysId = json_result["ordId"].asString();
	std::string okos = json_result["state"].asString();
	int os = OrderStatus2TD(okos);

	std::string ordKey = json_result["clOrdId"].asString();
	std::string totalOrdTraded = json_result["accFillSz"].asString();
	std::string instr = json_result["instId"].asString();
	std::string price = json_result["px"].asString();
	std::string qty = json_result["sz"].asString();

	bool is_outer_ord = false;
	int64_t key = -1;
	Order* ord = nullptr;

	if (ordKey.size() == 0 || ordKey[0] > '9' || ordKey[0] < '0') {//APPÊÖ¶¯µ¥
		is_outer_ord = true;
	}
	else {
		key = atoll(ordKey.data());
	}

	if (is_outer_ord) {
		SPDLOG_INFO("outSide Order,OnRtnOrder:key:{},instr:{},price:{},vol:{},trdVol:{}",
			ordKey.data(), instr.data(), price.data(), qty.data(), totalOrdTraded.data());
	}
	else {
		m_oms->get_order_from_id(key, ord);
		if (ord == nullptr) {
			SPDLOG_ERROR("trader_Okex::OnRtnOrder Can not find {} in oms", key);
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
		event.ExchangeIdx = EXCHANGE_OKEX;
		strcpy(event.InstrumentID, ord->InstrumentID);

		strcpy(ord->OrderSysID, ordSysId.c_str());
		if (false == is_final_status(ord->OrderStatus))
		{
			ord->OrderStatus = os;
			ord->TradeVolume = event.TotalTradeVolume;
			//SPDLOG_INFO("trader_Okex::OnRtnOrder ord {},os:{},trd:{}", ord->key, ord->OrderStatus, ord->TradeVolume);
		}

		m_rbwriter->push(event);
	}

	std::string tradeId = json_result["tradeId"].asString();
	SPDLOG_INFO("trader_Okex::OnRtnOrder,key:{},instr:{},price:{},vol:{},OS:{}",key, instr.data(), price.data(), qty.data(), okos);

	if (tradeId.length()>1)
	{ 
		{
			std::string instType = json_result["instType"].asString();
			std::string tradeVol = json_result["fillSz"].asString();
			std::string dir = json_result["side"].asString();
			int Direction = TD_LONG_DIRECTION;
			if ("sell" == dir)
				Direction = TD_SHORT_DIRECTION;

			std::string code = instr;
			if ("SPOT"== instType) {
				code = get_crypto_spot(instr, (char*)(STR_EXCHANGE_OKEX));
			}

			//SPDLOG_DEBUG("trader_Okex::update_trade,code:{},dir:{},vol:{}", code, Direction, tradeVol);
			update_trade(code.data(), Direction, atof(tradeVol.data()));
		}
		if (false == is_outer_ord) {
			OnRtnTrade(json_result, ord);
		}
	}
}

void trader_Okex::OnRtnTrade(Json::Value& json_result, Order* ord)
{
	std::string tradeVol = json_result["fillSz"].asString();
	std::string tradePrice = json_result["fillPx"].asString();
	std::string ordKey = json_result["clOrdId"].asString();
	std::string exeID = json_result["tradeId"].asString();
	std::string ordSysId = json_result["ordId"].asString();

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
	event.ExchangeIdx = EXCHANGE_OKEX;

	strcpy(event.InstrumentID, ord->InstrumentID);
	std::string instr = json_result["instId"].asString();
	if (0 != strcmp(instr.data(), ord->InstrumentID)) {
		SPDLOG_ERROR("instrument is not same:{},{}", instr, ord->InstrumentID);
	}

	m_rbwriter->push(event);

	SPDLOG_INFO("trader_Okex::OnRtnTrade,key:{},instr:{},price:{},tradevol:{},TradeID:{}",
		event.key, json_result["o"]["s"].asString().data(), event.TradePrice, event.LastTradeVolume, event.TradeID);
}

void trader_Okex::OnRspOrderInsert(Order* ord, std::string& errorID, std::string& msg)
{
	TBEvent event;
	event.eventType = TBRspOrderInsert;
	event.pubSubKey = ord->pubSubKey;
	event.key = ord->key;
	event.OrderStatus = OrderStatus_Error;
	event.Direction = ord->direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ord->InstrumentID);
	event.ErrorID = atoi(errorID.data());
	strncpy(event.errmsg, msg.data(), sizeof(event.errmsg) - 1);

	event.InstrumentHashVal = ord->hash_key;
	event.StrategyID = ord->StrategyID;
	event.SourceStrategyID = ord->SourceStrategyID;


	SPDLOG_ERROR("OnRspOrderInsert:errorID:{},errormsg:{},key:{},instru:{} ",
		errorID.data(), msg.data(), ord->key, ord->InstrumentID);


	if (ord->OrderStatus != OrderStatus_UnKnown) {
		SPDLOG_ERROR("trader_Okex::OnRspOrderInsert,order {} status:{} is not OrderStatus_UnKnown,this rsp maybe error",
			ord->key, ord->OrderStatus);
	}

	event.OrdPtr = ord;
	ord->OrderStatus = OrderStatus_Error;


	m_rbwriter->push(event);
}

void trader_Okex::OnRspOrderAction(Order* ord, std::string& errorID, std::string& msg)
{
	int64_t localOrderRef = ord->key;
	if (localOrderRef < 0)
	{
		SPDLOG_ERROR("recv unknown request's input order response {}", localOrderRef);
		return;
	}

	TBEvent event;
	event.eventType = TBRspOrderAction;
	event.key = localOrderRef;
	event.pubSubKey = ord->pubSubKey;
	event.OrderStatus = OrderStatus_Canceling;
	event.ErrorID = atoi(errorID.data());
	event.Direction = ord->direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ord->InstrumentID);
	strncpy(event.errmsg, msg.data(), sizeof(event.errmsg) - 1);

	event.InstrumentHashVal = ord->hash_key;
	event.StrategyID = ord->StrategyID;
	event.SourceStrategyID = ord->SourceStrategyID;

	event.OrdPtr = ord;
	m_rbwriter->push(event);

	SPDLOG_ERROR("OnRspOrderAction:errorID:{},errormsg:{},key:{},instru:{} ",
		errorID.data(), msg.data(), ord->key, ord->InstrumentID);
}

void trader_Okex::OnRspOrderInsert(const TBCryptoInputField* InputOrder, std::string& errorID, std::string& msg)
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
	event.ErrorID = atoi(errorID.data());
	strncpy(event.errmsg, msg.data(),sizeof(event.errmsg)-1);

	event.InstrumentHashVal = InputOrder->TradeItemHashVal;
	event.StrategyID = InputOrder->StrategyID;
	event.SourceStrategyID = InputOrder->SourceStrategyID;


	SPDLOG_ERROR("OnRspOrderInsert:errorID:{},errormsg:{},key:{},instru:{} ",
		errorID.data(), msg.data(), InputOrder->key, InputOrder->InstrumentID);

	Order* ord = nullptr;
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

void trader_Okex::OnRspOrderAction(const TBCryptoInputField* ActionOrder, std::string& errorID, std::string& msg)
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
	event.ErrorID = atoi(errorID.data());
	event.Direction = ActionOrder->Direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ActionOrder->InstrumentID);
	strncpy(event.errmsg, msg.data(), sizeof(event.errmsg) - 1);

	event.InstrumentHashVal = ActionOrder->TradeItemHashVal;
	event.StrategyID = ActionOrder->StrategyID;
	event.SourceStrategyID = ActionOrder->SourceStrategyID;

	Order* ord = nullptr;
	bool res = m_oms->get_order_from_id(localOrderRef, ord);
	if (res)
		event.OrdPtr = ord;
	else
		SPDLOG_ERROR("trader_Binance::OnRspOrderAction Can not find {} in oms", localOrderRef);
	m_rbwriter->push(event);

	SPDLOG_ERROR("OnRspOrderAction:errorID:{},errormsg:{},key:{},instru:{} ",
		errorID.data(), msg.data(), ActionOrder->key, ActionOrder->InstrumentID);
}
