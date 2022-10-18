#include "pionexcpp.h"
#include "simple_websocket.h"
#include <thread>

std::string old_apiKey = "2711YVKAmg1j5cFjNJa42HPOSZRkhT7QQt5rThkYdOyAEoweo4tyDCAqKlz9OUc6";
std::string old_secrertKey = "He6ME4swjZmyKiijc8u8UmPGSsAktxxhPm9HCSLOHOu2S8yCwTJsF4gwNN3KrtjE";

std::string apiKey = "3KfwFsVzviMmXGbfZB8Zrb5d31D1McqhhBAjkhutqnK9j3amR2g2zQRcD6vYWUcQvB";
std::string secrertKey = "I7yBFNDCZKGqmLoAXjCKTzknJAGAya0vf8qrWgAgxGyWRhCe6UOrtNImrvOG30AX";

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
	std::cout << jr["symbol"].asString() << std::endl;
	std::cout << jr["data"]["bids"][0].asString() << std::endl;
	std::cout << jr["data"]["asks"][0].asString() << std::endl;
	return 0;
}



void sub_depth()
{
	simple_websocket::init();
	simple_websocket::connect_pionex_public(on_rtn_depth);

	Json::Value jsObj;
	jsObj["op"] = "SUBSCRIBE";
	jsObj["topic"] = "DEPTH";
	jsObj["symbol"] = "BTC_USDT";
	jsObj["limit"] = 5;
	std::string jsonstr = jsObj.toStyledString();

	simple_websocket::send_pionex_lws_msg(jsonstr.data(), jsonstr.size());
}


int main(int argc, char* argv[])
{
	PionexCPP::init(apiKey, secrertKey);
	sub_depth();
	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
	return 0;
}

