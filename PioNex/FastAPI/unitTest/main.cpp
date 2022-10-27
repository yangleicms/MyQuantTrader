#include "pionexcpp.h"
#include "base_websocket.h"
#include <thread>

std::string apiKey = "3KfwFsVzviMmXGbfZB8Zrb5d31D1McqhhBAjkhutqnK9j3amR2g2zQRcD6vYWUcQvB";
std::string secrertKey = "I7yBFNDCZKGqmLoAXjCKTzknJAGAya0vf8qrWgAgxGyWRhCe6UOrtNImrvOG30AX";

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

//连接pionex ws服务并启动本地wscli
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
	//连接pionex ws服务并启动本地wscli

	std::thread t1(work);
	t1.detach();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	//通过公有留订阅行情
	//PionexCPP::sub_depth("ETH_USDT", cli_public_index);
	//通过私有留订阅指定合约的下单和成交回报
	PionexCPP::sub_order_ws("ETH_USDT", cli_private_index);
	//不要连续发送，会导致报错
	std::this_thread::sleep_for(std::chrono::seconds(1));
	PionexCPP::sub_fill_ws("ETH_USDT", cli_private_index);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	test_trade();
}

std::shared_ptr<WebSocketSSLClient> okex_pub;

int on_okex_depth(Json::Value& jr) {
	return 0;
}

void okex_work() {
	okex_pub = Webclient::connect(on_okex_depth, "/ws/v5/public", std::to_string(8443), "ws.okx.com", cli_public_index);
	Webclient::work();
}

void test_okex() {
	std::thread t1(okex_work);
	t1.detach();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	Json::Value jsObj,sub,args;
	sub["channel"] = "tickers";
	sub["instId"] = "ETH-USDT-SWAP";
	args.append(sub);

	jsObj["op"] = "subscribe";
	jsObj["args"] = args;

	std::string jsonstr = jsObj.toStyledString();
	std::cout << jsonstr << std::endl;
	okex_pub->Send(jsonstr);
}


int main(int argc, char* argv[])
{
	test_okex();

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
	return 0;
}

