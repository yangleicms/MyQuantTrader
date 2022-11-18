#include "trader_Pionex.h"
#include "cell_log.h"
#include <boost/algorithm/string.hpp>

using namespace cell::common;

std::string get_pionex_spot(std::string &code) {
	
	std::string symbol = "_";
	char* p = strstr((char*)code.data(), (char*)symbol.data());
	if (p == nullptr)
	{
		printf("trader_Pionex::error:can not find _ in instrument:%s,is it a SPOT instrumen?\n", code.data());
		std::this_thread::sleep_for(std::chrono::seconds(1));
		exit(0);
	}
	int offset = p - code.data();
	std::string sub = code.substr(0, offset);
	return sub;
}

int trader_Pionex::pionex_order_fill_event(Json::Value& jr)
{
	if (false == jr["op"].empty()) {
		std::string op = jr["op"].asString();
		if (op == "PING") {
			
			Json::Value jsObj;
			jsObj["op"] = "PONG";
			jsObj["timestamp"] = get_current_ms_epoch();
			std::string jsonstr = jsObj.toStyledString();
			this->OnPing(jsonstr);
		}
	}
	else if (false == jr["type"].empty()) {
		return 0;
	}
	else if (false == jr["topic"].empty())
	{
		std::string topic = jr["topic"].asString();
		//order rtn
		if (topic == "ORDER") {
			/*std::cout << "this is a orderRtn event\n";
			std::cout << "topic:" << topic << std::endl;
			std::cout << jr["symbol"].asString() << std::endl;
			std::cout << "orderId:" << jr["data"]["orderId"].asInt64() << std::endl;
			std::cout << "clientOrderId:" << jr["data"]["clientOrderId"].asString() << std::endl;
			std::cout << "price:" << jr["data"]["price"].asString() << std::endl;
			std::cout << "size:" << jr["data"]["size"].asString() << std::endl;
			std::cout << "filledSize:" << jr["data"]["filledSize"].asString() << std::endl;
			std::cout << "status:" << jr["data"]["status"].asString() << std::endl;*/
			this->OnRtnOrder(jr);
		}
		else if (topic == "FILL") {
			/*std::cout << "this is a FILL event\n";
			std::cout << "topic:" << topic << std::endl;
			std::cout << jr["symbol"].asString() << std::endl;
			std::cout << "orderId:" << jr["data"]["orderId"].asInt64() << std::endl;
			std::cout << "id:" << jr["data"]["id"].asInt64() << std::endl;
			std::cout << "role:" << jr["data"]["role"].asString() << std::endl;
			std::cout << "size:" << jr["data"]["size"].asString() << std::endl;
			std::cout << "price:" << jr["data"]["price"].asString() << std::endl;
			std::cout << "side:" << jr["data"]["side"].asString() << std::endl;*/
			this->OnRtnTrade(jr);
		}
	}
	return 0;
}

void trader_Pionex::OnPing(std::string& js)
{
	m_acc->send_lws_msg(js);
}

trader_Pionex::trader_Pionex(const std::string td_source_name, const int td_id) :
	crypto_trader(td_source_name, td_id){

	m_OS2TD["OPEN"] = OrderStatus_NoTradeQueueing;
	m_OS2TD["CLOSED"] = OrderStatus_Canceled;

	m_exchange_id = EXCHANGE_PIONEX;
}

void trader_Pionex::thread_start_websocket()
{
	std::string ws_url = m_acc->get_pionex_private_url();
	m_acc->wscli = Webclient::connect(std::bind(&trader_Pionex::pionex_order_fill_event, this, std::placeholders::_1), 
		ws_url.data(), std::to_string(WS_PORT), PIONEX_WS);
	m_WebSocketInit = true;
	Webclient::work();
}

bool trader_Pionex::insert_order(const TBCryptoInputField* InputOrder)
{
	std::string msg;
	Json::Value jr;

	SPDLOG_INFO("trader_Binance::insert_order,key:{},instr:{}", InputOrder->key, InputOrder->InstrumentID);

	Order* ord;
	bool findit = m_oms->get_order_from_id(InputOrder->key, ord);
	if (false == findit) {
		SPDLOG_ERROR("trader_Binance::insert_order,can not find order in oms: {} {}!", InputOrder->key, InputOrder->InstrumentID);
		return false;
	}
	update_order(InputOrder, ord);

	const char* symbol = InputOrder->InstrumentID;
	std::string side = "BUY";
	if (InputOrder->Direction == TD_SHORT_DIRECTION)
		side = "SELL";
	std::string _ty = "LIMIT";
	if (InputOrder->PriceType == TD_PriceType_AnyPrice)
		_ty = "MARKET";
	
	bool ioc = false;
	if (InputOrder->TimeCondition == TD_TimeCondition_IOC)
		ioc = true;

	m_acc->send_order(symbol,side.data(),
		_ty.data(),std::to_string(InputOrder->key).data(),InputOrder->VolumeTotal,
			InputOrder->LimitPrice,InputOrder->VolumeTotal,ioc,jr);

	bool res = jr["result"].asBool();
	if (false == res) {
		msg = jr["message"].asString();
		OnRspOrderInsert(InputOrder, -1, msg);
	}

	return true;
}

bool trader_Pionex::cancel_order(const TBCryptoInputField* ActionOrder)
{
	std::string msg;

	Order* order = nullptr;
	bool qres = m_oms->get_order_from_id(ActionOrder->key, order);

	if (false == qres) {
		SPDLOG_ERROR("trader_Binance::cancel_order, key={}. Has no OrigOrder.", ActionOrder->key);
		msg = "can not find orderID " + std::to_string(ActionOrder->key) + " in oms";
		this->OnRspOrderAction(ActionOrder, -1, msg);
		return false;
	}

	Json::Value jr;
	m_acc->cancel_order(ActionOrder->InstrumentID, atoll(order->OrderSysID), jr);
	bool res = jr["result"].asBool();
	if (res==false) {
		msg = jr["message"].asString();
		OnRspOrderAction(ActionOrder, -1, msg);
	}

	return true;
}

void trader_Pionex::OnRspOrderInsert(Order* ord, int errorID, std::string& msg)
{
	TBEvent event;
	event.eventType = TBRspOrderInsert;
	event.pubSubKey = ord->pubSubKey;
	event.key = ord->key;
	event.OrderStatus = OrderStatus_Error;
	event.Direction = ord->direction;
	event.ExchangeIdx = EXCHANGE_BINANCE;
	strcpy(event.InstrumentID, ord->InstrumentID);
	event.ErrorID = (errorID);
	strncpy(event.errmsg, msg.data(), sizeof(event.errmsg) - 1);

	event.InstrumentHashVal = ord->hash_key;
	event.StrategyID = ord->StrategyID;
	event.SourceStrategyID = ord->SourceStrategyID;


	SPDLOG_ERROR("OnRspOrderInsert:errorID:{},errormsg:{},key:{},instru:{} ",
		errorID, msg.data(), ord->key, ord->InstrumentID);


	if (ord->OrderStatus != OrderStatus_UnKnown) {
		SPDLOG_ERROR("trader_Pionex::OnRspOrderInsert,order {} status:{} is not OrderStatus_UnKnown,this rsp maybe error",
			ord->key, ord->OrderStatus);
	}

	event.OrdPtr = ord;
	ord->OrderStatus = OrderStatus_Error;


	m_rbwriter->push(event);
}

void trader_Pionex::OnRspOrderAction(Order* ord, int errorID, std::string& msg)
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
	event.ErrorID = (errorID);
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
		errorID, msg.data(), ord->key, ord->InstrumentID);
}

void trader_Pionex::OnRspOrderInsert(const TBCryptoInputField* InputOrder, int errorID, std::string& msg)
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
	event.ErrorID = (errorID);
	strncpy(event.errmsg, msg.data(), sizeof(event.errmsg) - 1);

	event.InstrumentHashVal = InputOrder->TradeItemHashVal;
	event.StrategyID = InputOrder->StrategyID;
	event.SourceStrategyID = InputOrder->SourceStrategyID;


	SPDLOG_ERROR("OnRspOrderInsert:errorID:{},errormsg:{},key:{},instru:{} ",
		errorID, msg.data(), InputOrder->key, InputOrder->InstrumentID);

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

void trader_Pionex::OnRspOrderAction(const TBCryptoInputField* ActionOrder, int errorID, std::string& msg)
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
	event.ErrorID = (errorID);
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
		errorID, msg.data(), ActionOrder->key, ActionOrder->InstrumentID);
}

bool trader_Pionex::init(Json::Value conf)
{
	crypto_trader::init(conf);
	m_apiKey = conf["api_key"].asString();
	m_secKey = conf["sec_key"].asString();
	std::string sub_code = conf["sub_code"].asString();

	SPDLOG_INFO("trader_Pionex init,apiKey:{},secKey:{}", m_apiKey, m_secKey);

	m_acc = new PionexAccount(m_apiKey, m_secKey);
	std::thread t1(std::bind(&trader_Pionex::thread_start_websocket, this));
	t1.swap(m_websocket_thread);
	cell_sleep_us(1000 * 1000);

	SPDLOG_INFO("trader_Pionex init WebSockets Fin");
	SPDLOG_INFO("trader_Pionex start sub order channel");
	sub_order_channel(sub_code);

	cell_sleep_us(1000);
	SPDLOG_INFO("trader_Pionex start qry position");
	query_all_pos();
	SPDLOG_INFO("trader_Pionex init Fin");
	return true;
}

void trader_Pionex::sub_order_channel(std::string strr)
{
	if (strr.size() < 1) {
		SPDLOG_ERROR("trader_Pionex::sub_order_channel fail,codestrr:{}", strr);
		std::cout << "trader_Pionex::sub_order_channel WebSocket Fail:" << strr << std::endl;
		cell_sleep_us(1000 * 1000);
		exit(0);
	}

	std::vector<std::string> vec;
	boost::split(vec, strr, boost::is_any_of(","));

	Json::Value jsObj, sub, args;

	for (auto& it : vec) {
		SPDLOG_DEBUG("trader_Pionex::sub_order_channel,sub:{}", it);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		m_acc->sub_order_ws(it);
		//不要连续发送，会导致报错
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		m_acc->sub_fill_ws(it);
	}
	
}

void trader_Pionex::query_all_pos()
{
	Json::Value jv;
	m_acc->get_all_pos(jv);
	bool res = jv["result"].asBool();
	if (false == res) {
		SPDLOG_ERROR("trader_Pionex::query_all_pos Fail");
	}

	auto dat = jv["data"]["balances"];
	int len = dat.size();
	for (int i = 0; i < len; ++i) {
		std::string symbol = dat[i]["coin"].asString();
		auto pos = get_or_create_trader_pos(symbol.data());
		std::string free = dat[i]["free"].asString();
		std::string frozen = dat[i]["frozen"].asString();

		pos->tot_net_pos = atof(free.data()) + atof(frozen.data());
		pos->freeze = atof(frozen.data());

		SPDLOG_INFO("OnRspQryPosition Ins={},positon:{},froze:{} ", 
			symbol, pos->tot_net_pos, pos->freeze);
	}
}

void trader_Pionex::OnRtnOrder(Json::Value& jv)
{
	auto json_result = jv["data"];
	std::string ordSysId = json_result["orderId"].asString();
	std::string ordKey = json_result["clientOrderId"].asString();
	std::string totalOrdTraded = json_result["filledSize"].asString();
	std::string totalTurnOver = json_result["filledAmount"].asString();
	std::string instr = json_result["symbol"].asString();
	std::string price = json_result["price"].asString();
	std::string qty = json_result["size"].asString();

	std::string status = json_result["status"].asString();
	int os = OrderStatus_NoTradeQueueing;
	double tot_trade = atof(totalOrdTraded.data());
	double tot_qty = atof(qty.data());
	if (status != "OPEN") {
		if (is_zero(tot_trade - tot_qty)) {
			os = OrderStatus_AllTraded;
		}
		else {
			os = OrderStatus_Canceled;
		}
	}

	bool is_outer_ord = false;
	int64_t key = -1;
	Order* ord = nullptr;

	if (ordKey.size() == 0 || ordKey[0] > '9' || ordKey[0] < '0') {//APP手动单
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
			SPDLOG_ERROR("trader_Pionex::OnRtnOrder Can not find {} in oms", key);
			return;
		}

		TBEvent event;
		event.key = key;
		event.pubSubKey = ord->pubSubKey;
		event.eventType = TBRtnOrder;
		event.TotalTradeVolume = atof(totalOrdTraded.data());
		event.TotalTurnOver = atof(totalTurnOver.data());
		event.OrderStatus = os;
		event.StrategyID = ord->StrategyID;
		event.SourceStrategyID = ord->SourceStrategyID;
		event.InstrumentHashVal = ord->hash_key;
		event.OrdPtr = ord;
		event.Direction = ord->direction;
		event.ExchangeIdx = EXCHANGE_PIONEX;
		strcpy(event.InstrumentID, ord->InstrumentID);

		strcpy(ord->OrderSysID, ordSysId.c_str());
		if (false == is_final_status(ord->OrderStatus))
		{
			ord->OrderStatus = os;
			ord->TradeVolume = event.TotalTradeVolume;
			ord->TotalTurnOver = event.TotalTurnOver;
			SPDLOG_INFO("trader_Pionex::OnRtnOrder ord {},os:{},trd:{}", ord->key, ord->OrderStatus, ord->TradeVolume);
		}
		
		m_sysID2localID[ordSysId] = key;

		m_rbwriter->push(event);
	}

	std::string tradeId = json_result["tradeId"].asString();
	SPDLOG_INFO("trader_Pionex::OnRtnOrder,key:{},instr:{},price:{},vol:{},OS:{}", key, instr.data(), price.data(), qty.data(), status);

}

void trader_Pionex::OnRtnTrade(Json::Value& jv)
{
	auto json_result = jv["data"];
	std::string tradeVol = json_result["size"].asString();
	std::string tradePrice = json_result["price"].asString();
	std::string exeID = json_result["id"].asString();
	std::string ordSysId = json_result["orderId"].asString();

	std::string side = json_result["side"].asString();
	std::string symbol = json_result["symbol"].asString();
	int Direction = TD_LONG_DIRECTION;
	if ("SELL" == side)
		Direction = TD_SHORT_DIRECTION;

	std::string code = get_pionex_spot(symbol);

	SPDLOG_DEBUG("trader_Okex::update_trade,code:{},dir:{},vol:{}", code, Direction, tradeVol);
	update_trade(code.data(), Direction, atof(tradeVol.data()));

	auto it = m_sysID2localID.find(ordSysId);
	if (it == m_sysID2localID.end()) {
		SPDLOG_ERROR("trader_Pionex::OnRtnTrade,can not find local key from sysID:{},mabey it is a out ord", ordSysId);
		return;
	}

	int64_t key = it->second;
	Order* ord = nullptr;
	m_oms->get_order_from_id(key, ord);
	if (ord == nullptr) {
		SPDLOG_ERROR("trader_Pionex::OnRtnTrade Can not find {} in oms", key);
		return;
	}

	if (false == is_final_status(ord->OrderStatus))
	{
		ord->TradeVolume += atof(tradeVol.data());
		ord->TotalTurnOver += atof(tradeVol.data())* atof(tradePrice.data());
		SPDLOG_INFO("trader_Pionex::OnRtnOrder ord {},os:{},trd:{}", ord->key, ord->OrderStatus, ord->TradeVolume);
	}

	TBEvent event;
	event.eventType = TBRtnTradeInfo;
	event.pubSubKey = ord->pubSubKey;
	event.LastTradeVolume = atof(tradeVol.data());
	event.TradePrice = atof(tradePrice.data());
	strncpy(event.TradeID, exeID.data(),strlen(event.TradeID));
	event.key = key;
	event.InstrumentHashVal = ord->hash_key;
	event.StrategyID = ord->StrategyID;
	event.SourceStrategyID = ord->SourceStrategyID;
	event.OrdPtr = ord;
	event.Direction = ord->direction;
	event.ExchangeIdx = EXCHANGE_PIONEX;
	event.TotalTradeVolume = ord->TradeVolume;
	event.TotalTurnOver = ord->TotalTurnOver;

	strcpy(event.InstrumentID, ord->InstrumentID);
	std::string instr = json_result["symbol"].asString();
	if (0 != strcmp(instr.data(), ord->InstrumentID)) {
		SPDLOG_ERROR("instrument is not same:{},{}", instr, ord->InstrumentID);
	}

	m_rbwriter->push(event);

	SPDLOG_INFO("trader_Pionex::OnRtnTrade,key:{},instr:{},price:{},tradevol:{},TradeID:{}",
		event.key, symbol, event.TradePrice, event.LastTradeVolume, event.TradeID);
}