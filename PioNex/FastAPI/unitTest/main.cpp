#include "pionexcpp.h"
#include "okexcpp.h"
#include "base_websocket.h"
#include "utils.h"
#include <thread>

std::string apiKey = "3KfwFsVzviMmXGbfZB8Zrb5d31D1McqhhBAjkhutqnK9j3amR2g2zQRcD6vYWUcQvB";
std::string secrertKey = "I7yBFNDCZKGqmLoAXjCKTzknJAGAya0vf8qrWgAgxGyWRhCe6UOrtNImrvOG30AX";

std::string ok_key = "2";
std::string ok_sec = "8";
std::string ok_pswd = "!";

int cli_public_index = -1;
int cli_private_index = -1;

void test_trade()
{
	Json::Value jr;
	std::string localID = std::to_string(pionex_get_current_ms_epoch());
	/*std::string localID = "1765885567201";
	std::string localID1 = "1765885567203";*/

	PionexCPP::send_order("ETH_USDT", "SELL", "LIMIT", localID.data(), 0.01, 1350, 0, false, jr);

	uint64_t ordID = jr["data"]["orderId"].asInt64();
	//std::cout<<ordID<<"-"<<jr["data"]["orderId"].asString()<<std::endl;
	//std::string strID = jr["data"]["orderId"].asString();

	//uint64_t ordID = 11020405198194958;
	std::cout << "get localord:" << localID << std::endl;
	PionexCPP::get_order("ETH_USDT", localID.data(), jr);

	std::cout << "get sysord:" << ordID << std::endl;
	PionexCPP::get_order("ETH_USDT", ordID, jr);

	std::cout << "cancel order:" << ordID << std::endl;
	PionexCPP::cancel_order("ETH_USDT", ordID, jr);
}

int on_rtn_depth(Json::Value &jr) 
{
	if (false == jr["topic"].empty()) {
		std::string topic = jr["topic"].asString();
		std::cout << "topic:" << topic << std::endl;
		std::cout << jr["symbol"].asString() << std::endl;
		if (topic == "DEPTH") {
			std::cout << jr["data"]["bids"][0][0].asString() << std::endl;
			std::cout << jr["data"]["asks"][0][0].asString() << std::endl;
		}
	}
	else if (false == jr["op"].empty()) {
		std::string op = jr["op"].asString();
		if (op == "PING") {
			auto pub_cli = Webclient::get_cli(cli_public_index);
			Json::Value jsObj;
			jsObj["op"] = "PONG";
			jsObj["timestamp"] = pionex_get_current_ms_epoch();
			std::string jsonstr = jsObj.toStyledString();
			pub_cli->Send(jsonstr);
		}
	}
	return 0;
}

int on_rtn_order_and_fill(Json::Value& jr)
{
	if (false == jr["op"].empty()) {
		std::string op = jr["op"].asString();
		if (op == "PING") {
			auto pri_cli = Webclient::get_cli(cli_private_index);
			Json::Value jsObj;
			jsObj["op"] = "PONG";
			jsObj["timestamp"] = pionex_get_current_ms_epoch();
			std::string jsonstr = jsObj.toStyledString();
			pri_cli->Send(jsonstr);
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
			std::cout << "this is a orderRtn event\n";
			std::cout << "topic:" << topic << std::endl;
			std::cout << jr["symbol"].asString() << std::endl;
			std::cout << "orderId:" << jr["data"]["orderId"].asInt64() << std::endl;
			std::cout << "clientOrderId:" << jr["data"]["clientOrderId"].asString() << std::endl;
			std::cout << "price:" << jr["data"]["price"].asString() << std::endl;
			std::cout << "size:" << jr["data"]["size"].asString() << std::endl;
			std::cout << "filledSize:" << jr["data"]["filledSize"].asString() << std::endl;
			std::cout << "status:" << jr["data"]["status"].asString() << std::endl;
		}
		else if (topic == "FILL") {
			std::cout << "this is a FILL event\n";
			std::cout << "topic:" << topic << std::endl;
			std::cout << jr["symbol"].asString() << std::endl;
			std::cout << "orderId:" << jr["data"]["orderId"].asInt64() << std::endl;
			std::cout << "id:" << jr["data"]["id"].asInt64() << std::endl;
			std::cout << "role:" << jr["data"]["role"].asString() << std::endl;
			std::cout << "size:" << jr["data"]["size"].asString() << std::endl;
			std::cout << "price:" << jr["data"]["price"].asString() << std::endl;
			std::cout << "side:" << jr["data"]["side"].asString() << std::endl;
		}
	}
	return 0;
}

//¿¿pionex ws¿¿¿¿¿¿¿wscli
void connect_pionex_PubAndPrivate_ws(CB pub, CB pri, int& cli_public_index, int& cli_private_index)
{
	//connect pionex public ws
	auto pub_cli = Webclient::connect(pub, "/wsPub", std::to_string(WS_PORT), PIONEX_WS, cli_public_index);
	//connect pionex private ws
	std::string path = PionexCPP::get_pionex_private_url();
	auto pri_cli = Webclient::connect(pri, path.data(), std::to_string(WS_PORT), PIONEX_WS, cli_private_index);
	//start boost ioc srv
	Webclient::work();
}

void work() {
	connect_pionex_PubAndPrivate_ws(on_rtn_depth, on_rtn_order_and_fill, cli_public_index, cli_private_index);
}

void test_pionex_order()
{
	PionexCPP::init(apiKey, secrertKey);
	//¿¿pionex ws¿¿¿¿¿¿¿wscli

	std::thread t1(work);
	t1.detach();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	//¿¿¿¿¿¿¿¿¿
	//PionexCPP::sub_depth("ETH_USDT", cli_public_index);
	//¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿
	PionexCPP::sub_order_ws("ETH_USDT", cli_private_index);
	//¿¿¿¿¿¿¿¿¿¿¿¿
	std::this_thread::sleep_for(std::chrono::seconds(1));
	PionexCPP::sub_fill_ws("ETH_USDT", cli_private_index);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	test_trade();
}

std::shared_ptr<WebSocketSSLClient> okex_pub;
std::shared_ptr<WebSocketSSLClient> okex_pri;

int on_okex_depth(Json::Value& jr) {
	return 0;
}



class okex_api 
{
public:
	okex_api() {}
	~okex_api() {}

	void okex_work() {
		//okex_pub = Webclient::connect(on_okex_depth, "/ws/v5/public", std::to_string(8443), "ws.okx.com", cli_public_index);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		okex_pri = Webclient::connect(on_okex_depth, "/ws/v5/private", std::to_string(8443),
			"ws.okx.com", cli_public_index,std::bind(&okex_api::on_reconn,this));
		Webclient::work();
	}

	void test_okex_md() {
		std::thread t1(std::bind(&okex_api::okex_work,this));
		t1.detach();
		std::this_thread::sleep_for(std::chrono::seconds(1));

		Json::Value jsObj, sub, args;
		sub["channel"] = "tickers";
		sub["instId"] = "ETH-USDT-SWAP";
		args.append(sub);

		jsObj["op"] = "subscribe";
		jsObj["args"] = args;

		std::string jsonstr = jsObj.toStyledString();
		std::cout << jsonstr << std::endl;
		okex_pub->Send(jsonstr);
	}

	void auth() {
		std::string tp = std::to_string(get_current_ms_epoch() / 1000);
		std::string strr = tp + "GET/users/self/verify";

		Json::Value jsObj, sub, args;
		sub["apiKey"] = ok_key.data();
		sub["passphrase"] = ok_pswd.data();
		sub["timestamp"] = tp;
		sub["sign"] = get_okex_sign(ok_sec.data(), strr.data());
		args.append(sub);

		jsObj["op"] = "login";
		jsObj["args"] = args;
		std::string jsonstr = jsObj.toStyledString();
		std::cout << jsonstr << std::endl;
		okex_pri->Send(jsonstr);
	}

	void sub_channel() {
		Json::Value jsObj, sub, args;
		jsObj["op"] = "subscribe";
		sub["channel"] = "orders";
		sub["instType"] = "SWAP";
		sub["instId"] = "MATIC-USDT-SWAP";
		args.append(sub);
		sub["channel"] = "orders";
		sub["instType"] = "SPOT";
		sub["instId"] = "MATIC-USDT";
		args.append(sub);

		jsObj["args"] = args;
		std::string jsonstr = jsObj.toStyledString();
		std::cout << jsonstr << std::endl;
		okex_pri->Send(jsonstr);
	}
	
	void send_ord()
	{
		Json::Value jsObj, sub, args;
		jsObj["id"] = "1099";
		jsObj["op"] = "order";

		sub["side"] = "buy";
		sub["instId"] = "MATIC-USDT-SWAP";
		sub["tdMode"] = "isolated";
		sub["ordType"] = "limit";
		sub["sz"] = "1";
		sub["px"] = "0.936";
		sub["clOrdId"] = "66666";

		args.append(sub);
		jsObj["args"] = args;
		std::string jsonstr = jsObj.toStyledString();
		std::cout << jsonstr << std::endl;
		okex_pri->Send(jsonstr);
	}

	void cancel_ord() {
		Json::Value jsObj, sub, args;
		jsObj["id"] = "1033";
		jsObj["op"] = "cancel-order";

		sub["instId"] = "MATIC-USDT-SWAP";
		sub["clOrdId"] = "66666";

		args.append(sub);
		jsObj["args"] = args;
		std::string jsonstr = jsObj.toStyledString();
		std::cout << jsonstr << std::endl;
		okex_pri->Send(jsonstr);
	}

	void on_reconn() {
		std::cout << "OKEX private ws Reconn,login\n";
		auth();
		return;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "OKEX sub\n";
		sub_channel();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "OKEX sendOrd\n";
		send_ord();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "OKEX cancelOrd\n";
		cancel_ord();
	}

	void test_okex_private() {
		std::thread t1(std::bind(&okex_api::okex_work, this));
		t1.detach();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		auth();
		return;
	}
};

int main(int argc, char* argv[])
{
	okex_api api;
	api.test_okex_private();
	//test_okex_private();
	//OkexCPP::init(ok_key,ok_sec,ok_pswd);
	//OkexCPP::get_pos();
	//Json::Value res;
	//OkexCPP::get_order(res,"MATIC-USDT-SWAP","","507602064474812435");
	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
	return 0;
}


