#include "pionexcpp.h"
#include <thread>

std::string old_apiKey = "2711YVKAmg1j5cFjNJa42HPOSZRkhT7QQt5rThkYdOyAEoweo4tyDCAqKlz9OUc6";
std::string old_secrertKey = "He6ME4swjZmyKiijc8u8UmPGSsAktxxhPm9HCSLOHOu2S8yCwTJsF4gwNN3KrtjE";

std::string apiKey = "3KfwFsVzviMmXGbfZB8Zrb5d31D1McqhhBAjkhutqnK9j3amR2g2zQRcD6vYWUcQvB";
std::string secrertKey =  "I7yBFNDCZKGqmLoAXjCKTzknJAGAya0vf8qrWgAgxGyWRhCe6UOrtNImrvOG30AX";



int main(int argc,char *argv[])
{
	PionexCPP::init(apiKey, secrertKey);
	Json::Value jr;
	PionexCPP::send_order("ETH_USDT", "BUY", "LIMIT", "1024", 0.01, 1200, 0, false, jr);

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
  return 0;
}

