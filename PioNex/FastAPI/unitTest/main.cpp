#include "pionexcpp.h"
#include "base_websocket.h"
#include <thread>

std::string old_apiKey = "2711YVKAmg1j5cFjNJa42HPOSZRkhT7QQt5rThkYdOyAEoweo4tyDCAqKlz9OUc6";
std::string old_secrertKey = "He6ME4swjZmyKiijc8u8UmPGSsAktxxhPm9HCSLOHOu2S8yCwTJsF4gwNN3KrtjE";

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
	else if (false == jr["topic"].empty())
	{
		std::string topic = jr["topic"].asString();
		std::cout << "topic:" << topic << std::endl;
		std::cout << jr["symbol"].asString() << std::endl;
		std::cout << "orderId:" << jr["data"]["orderId"].asInt64() << std::endl;
		//order rtn
		if (topic == "ORDER") {
			std::cout << "this is a orderRtn event\n" << std::endl;
			std::cout << "clientOrderId:" << jr["data"]["clientOrderId"].asString() << std::endl;
			std::cout << "price:" << jr["data"]["price"].asString() << std::endl;
			std::cout << "size:" << jr["data"]["size"].asString() << std::endl;
			std::cout << "filledSize:" << jr["data"]["filledSize"].asString() << std::endl;
			std::cout << "status:" << jr["data"]["status"].asString() << std::endl;
		}
		else if (topic == "FILL") {
			std::cout << "this is a FILL event\n" << std::endl;
			std::cout << "id:" << jr["data"]["id"].asInt64() << std::endl;
			std::cout << "role:" << jr["data"]["role"].asString() << std::endl;
			std::cout << "size:" << jr["data"]["size"].asString() << std::endl;
			std::cout << "price:" << jr["data"]["price"].asString() << std::endl;
			std::cout << "side:" << jr["data"]["side"].asString() << std::endl;
		}
	}
}

//����pionex ws������������wscli
void connect_pionex_PubAndPrivate_ws()
{
	//connect pionex public ws
	auto pub_cli = Webclient::connect(on_rtn_depth, "/wsPub", std::to_string(WS_PORT), PIONEX_WS, cli_public_index);
	//connect pionex private ws
	std::string path = PionexCPP::get_pionex_private_url();
	auto pri_cli = Webclient::connect(on_rtn_order_and_fill, path.data(), std::to_string(WS_PORT), PIONEX_WS, cli_private_index);
	//start boost ioc srv
	Webclient::work();
}

//ͨ����������������
void sub_depth()
{
	//sub BTC_USDT md
	auto pub_cli = Webclient::get_cli(cli_public_index);
	Json::Value jsObj;
	jsObj["op"] = "SUBSCRIBE";
	jsObj["topic"] = "DEPTH";
	jsObj["symbol"] = "ETH_USDT";
	jsObj["limit"] = 5;
	std::string jsonstr = jsObj.toStyledString();
	pub_cli->Send(jsonstr);

}

//ͨ��˽��������ָ����Լ���µ��ͳɽ��ر�
void sub_callback() {
	auto private_cli = Webclient::get_cli(cli_private_index);
	Json::Value jsObj,fillObj;
	jsObj["op"] = "SUBSCRIBE";
	jsObj["topic"] = "ORDER";
	jsObj["symbol"] = "ETH_USDT";
	std::string jsonstr = jsObj.toStyledString();
	private_cli->Send(jsonstr);

	fillObj["op"] = "SUBSCRIBE";
	fillObj["topic"] = "FILL";
	fillObj["symbol"] = "ETH_USDT";
	jsonstr = fillObj.toStyledString();
	private_cli->Send(jsonstr);
}


int main(int argc, char* argv[])
{
	PionexCPP::init(apiKey, secrertKey);
	//����pionex ws������������wscli
	std::thread t1(connect_pionex_PubAndPrivate_ws);
	t1.detach();
	sleep(1);
	//ͨ����������������
	//sub_depth();
	//ͨ��˽��������ָ����Լ���µ��ͳɽ��ر�
	sub_callback();
	sleep(1);
	test_trade();

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
	return 0;
}

