#pragma once
#ifndef BASE_WEBSOCKET_H
#define BASE_WEBSOCKET_H
//std lib
#include <string>
#include <map>
#include <list>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>

//third lib
#include <json/json.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <workflow/HttpMessage.h>
#include <workflow/WFTaskFactory.h>
#include "WebSocketSSLClient.h"


#define BINANCE_WS_HOST "stream.binance.com"
#define BINANCE_WS_UB_HOST "fstream.binance.com"
#define BINANCE_WS_CB_HOST "dstream.binance.com"

#define WS_PORT 443
#define BINANCE_CB_POSRSP_ID 999
#define PINGPONG_INTERVAL 10

#define PIONEX_WS "ws.pionex.com"

typedef std::function<int(Json::Value&)> CB;

static std::string get_workThread_status(int st) {
	switch (st)
	{
	case -1:
		return "WFT_STATE_UNDEFINED";
	case CS_STATE_SUCCESS:
		return "WFT_STATE_SUCCESS";
	case CS_STATE_TOREPLY:
		return "WFT_STATE_TOREPLY";
	case CS_STATE_TOREPLY + 1:
		return "WFT_STATE_NOREPLY";
	case CS_STATE_ERROR:
		return "WFT_STATE_SYS_ERROR";
	case 65:
		return "WFT_STATE_SSL_ERROR";
	case 66:
		return "WFT_STATE_DNS_ERROR";
	case 67:
		return "WFT_STATE_TASK_ERROR";
	case CS_STATE_STOPPED:
		return "WFT_STATE_ABORTED";
	default:
		return "WFT_UNKOWN";
		break;
	}
}

class single_con
{
public:
	static single_con* get_instance()
	{
		static single_con res;
		return &res;
	}

	boost::asio::io_context m_ioc;
	std::atomic<int> m_index;
	std::atomic<int> m_work;
	std::map<int, std::shared_ptr<WebSocketSSLClient>> m_dat;

private:
	single_con() { m_index = 0; m_work.store(0); }
	~single_con() {}
};

class WSClient :public WebSocketClientApi
{
public:
	virtual void OnConnected(int id) {
		++m_conn_num;
		printf("OnConnected,id:%d,conn_num:%d\n", id, m_conn_num);
		if (m_conn_num > 1 && m_OnReconn) {
			std::thread t1(m_OnReconn);
			t1.detach();
		}
	};

	virtual void OnClosed(std::string uri) {
		std::string msg = uri + ":ws has closed";
		printf("%s\n", msg.data());
		
	}
	virtual void OnReconnect(std::string msg) {
		printf("%s\n", msg.data());
	}

	virtual bool OnSubscribed(char* data, size_t len) {
		return true;
	}
	virtual bool OnUnsubscribed(char* data, size_t len) {
		return true;
	}
	virtual bool OnDataReady(char* data, size_t len, int id) {

		try
		{
			if (len < 4){
				std::cout << "error rcv len,str:" << data << std::endl;
			}
			std::cout<<data<<std::endl;

			Json::CharReaderBuilder b;
			std::unique_ptr<Json::CharReader> reader(b.newCharReader());
			Json::Value json_result;
			JSONCPP_STRING errs;
			reader->parse(data, data + len, &json_result, &errs);
			cb(json_result);
		}
		catch (std::exception& e)
		{
			printf("<binance::Websocket::event_cb> Error parsing incoming message : %s\n", e.what());
			return 1;
		}
		return true;
	}
	virtual std::string GenerateSubscribeData(std::unordered_set<std::string> pairs) {
		return "";
	}
	virtual std::string GenerateUnsubscribeData(std::unordered_set<std::string> pairs) {
		return"";
	}
	CB cb;
	int m_conn_num = 0;
	std::function<void()> m_OnReconn;
};

static int get_dur_from_ubiq_tradingday_begin()
{
	time_t timep;
	time(&timep);
	struct tm tm;
	tm = *localtime(&timep);

	if (tm.tm_hour >= 18)
		return ((tm.tm_hour - 18) * 3600 + tm.tm_min * 60 + tm.tm_sec) * 1000;
	else
		return ((tm.tm_hour + 6) * 3600 + tm.tm_min * 60 + tm.tm_sec) * 1000;
}

static void get_ms_epoch(long long& nowtp, long long& daybegin)
{
	time_t now = time(0);
	tm* gmtm = gmtime(&now);
	nowtp = now * 1000;
	//daybegin = nowtp - (gmtm->tm_hour * 3600 + gmtm->tm_min * 60 + gmtm->tm_sec) * 1000;
	daybegin = nowtp - get_dur_from_ubiq_tradingday_begin();
}

static std::string Pionex_DoubletoString(double val,int prec)
{
	std::ostringstream out;
	out.precision(prec);
	out.setf(std::ios_base::fixed);
	out << val;
	return out.str();
}

static int http_async(const std::string& url, std::map<std::string,std::string>& extra_http_header,
		const std::string& body, const std::string& action, const std::string &keyinfo, std::function<void(std::string&, std::string&)> callback)
{
		auto task = WFTaskFactory::create_http_task(url, 0, 0, [callback,&keyinfo](WFHttpTask* t)
		{
				auto state = t->get_state();

				if (state != WFT_STATE_SUCCESS)
				{
					std::string msg = "workThread Lib Http TaskError:" + get_workThread_status(state);
					std::string info = keyinfo;
					callback(msg, info);
					if (state == CS_STATE_ERROR) {
					}

					return -1;
				}
				auto resp = t->get_resp();

				const void* p;
				size_t len;
				resp->get_parsed_body(&p, &len);

				std::string content(reinterpret_cast<const char*>(p), len);
				std::string info = keyinfo;
				callback(content, info);

		});

		auto req = task->get_req();
		req->set_method(action);
		req->append_output_body(body.data(), body.size());

		for (auto it: extra_http_header) {
			req->add_header_pair(it.first,it.second);
		}
		task->start();
		return 0;
}


class SmartCURL
{
	CURL* curl;
public:

	CURL* get() { return curl; }
	SmartCURL()
	{
		curl = curl_easy_init();
	}
	~SmartCURL()
	{
		curl_easy_cleanup(curl);
	}
};

static size_t getCurlCb(void* content, size_t size, size_t nmemb, std::string* buffer)
{
	char* p = (char*)content;
	size_t newLength = size * nmemb;
	size_t oldLength = buffer->size();
	buffer->resize(oldLength + newLength);
	std::copy((char*)content, (char*)content + newLength, buffer->begin() + oldLength);
	return newLength;
}

static int getCurlWithHeader(std::string& str_result, const std::string& url, const std::vector<std::string>& extra_http_header,
	const std::string& post_data, const std::string& action)
{
	SmartCURL curl;

	if (curl.get() != NULL)
	{
		curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, getCurlCb);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &str_result);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, false);
		if (curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, true) != CURLE_OK)
		{
			return -1;
		}

		if (extra_http_header.size() > 0)
		{
			struct curl_slist* chunk = NULL;
			for (int i = 0; i < extra_http_header.size(); i++)
				chunk = curl_slist_append(chunk, extra_http_header[i].c_str());

			curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk);
		}

		if (post_data.size() > 0 || action == "POST" || action == "PUT" || action == "DELETE")
		{
			if (action == "PUT" || action == "DELETE")
				curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, action.c_str());
			curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, post_data.c_str());
		}
		CURLcode res;
		try
		{
			res = curl_easy_perform(curl.get());
			if ((res == CURLE_OK))// && (res_code == 200 || res_code == 201))
			{

			}
			else{
				return -1;
			}
		}
		catch (std::exception& e){
			printf(e.what());
			return -1;
		}
	}
	return 0;
}

static int getCurl(std::string& result_json, const std::string& url)
{
	std::vector<std::string> v;
	std::string action = "GET";
	std::string post_data = "";
	return getCurlWithHeader(result_json, url, v, post_data, action);
}

class Webclient
{
public:
	static bool curl_init()
	{
		CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
		if (CURLE_OK != res)
		{
			return false;
		}
		return true;
	}

	static std::shared_ptr<WebSocketSSLClient> get_cli(int id) {
		auto it = single_con::get_instance()->m_dat.find(id);
		if (it != single_con::get_instance()->m_dat.end())
			return it->second;
		return nullptr;
	}

	static std::shared_ptr<WebSocketSSLClient> connect(CB cb, const char* path, std::string port, std::string host,int &id, std::function<void()> rc = nullptr) {

		id = ++(single_con::get_instance()->m_index);
		WebSocketClientApi* api = new WSClient();
		WSClient* rap = static_cast<WSClient*>(api);
		rap->cb = cb;
		if (rc) {
			rap->m_OnReconn = rc;
		}

		std::shared_ptr<WebSocketSSLClient> cli =
			std::make_shared<WebSocketSSLClient>(id, single_con::get_instance()->m_ioc, host, port, api);
		cli->Connect(path);

		single_con::get_instance()->m_dat[id] = cli;
		return cli;
	}

	static std::shared_ptr<WebSocketSSLClient> connect_endpoint_ub(CB cb, const char* path){
		std::string port = std::to_string(WS_PORT);
		std::string host = BINANCE_WS_UB_HOST;
		int id = ++(single_con::get_instance()->m_index);
		WebSocketClientApi* api = new WSClient();
		WSClient* rap = static_cast<WSClient*>(api);
		rap->cb = cb;

		std::shared_ptr<WebSocketSSLClient> cli = 
			std::make_shared<WebSocketSSLClient>(id, single_con::get_instance()->m_ioc, host, port, api);
		cli->Connect(path);

		single_con::get_instance()->m_dat[id] = cli;
		return cli;
	}

	static std::shared_ptr<WebSocketSSLClient> connect_endpoint_cb(CB cb, const char* path) {
		std::string port = std::to_string(WS_PORT);
		std::string host = BINANCE_WS_CB_HOST;
		int id = ++(single_con::get_instance()->m_index);
		WebSocketClientApi* api = new WSClient();
		WSClient* rap = static_cast<WSClient*>(api);
		rap->cb = cb;

		std::shared_ptr<WebSocketSSLClient> cli =
			std::make_shared<WebSocketSSLClient>(id, single_con::get_instance()->m_ioc, host, port, api);
		cli->Connect(path);

		single_con::get_instance()->m_dat[id] = cli;
		return cli;
	}

	static std::shared_ptr<WebSocketSSLClient> connect_endpoint_cash(CB cb, const char* path) {
                std::string port = std::to_string(WS_PORT);
                std::string host = BINANCE_WS_HOST;
                int id = ++(single_con::get_instance()->m_index);
                WebSocketClientApi* api = new WSClient();
                WSClient* rap = static_cast<WSClient*>(api);
                rap->cb = cb;

                std::shared_ptr<WebSocketSSLClient> cli =
                        std::make_shared<WebSocketSSLClient>(id, single_con::get_instance()->m_ioc, host, port, api);
                cli->Connect(path);

                single_con::get_instance()->m_dat[id] = cli;
                return cli;
        }

	static void work(){
		++single_con::get_instance()->m_work;
		if (single_con::get_instance()->m_work != 1)
			return;

		while (1) {
			if (single_con::get_instance()->m_work==1)
				single_con::get_instance()->m_ioc.run();
		}
	}

};

#endif

