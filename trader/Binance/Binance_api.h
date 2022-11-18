#pragma once
#ifndef BINANCE_API_H
#define BINANCE_API_H
#include "base_websocket.h"
#include "common.h"
//#include "cell_log.h"

using namespace cell::common;

#define STR_HOST_NAME_MAX 256
#define BINANCE_CASE_STR(err) case err : { static const std::string str_##err = #err; return str_##err.c_str(); }

#define CHECK_SERVER_ERR(result)                                     \
    do {                                                             \
                                                                     \
        bool err = false;                                            \
        if (result.isObject())                                       \
        {                                                            \
            const std::vector<std::string> keys = result.getMemberNames();     \
            for (int i = 0, e = keys.size(); i < e; i++)             \
            {                                                        \
                const std::string& ikey = keys[i];                        \
                if (ikey == "code")                                  \
                {                                                    \
                    for (int j = 0, e = keys.size(); j < e; j++)     \
                    {                                                \
                        const std::string& jkey = keys[j];                \
                        if (jkey == "msg") { err = true; break; }    \
                    }                                                \
                }                                                    \
                                                                     \
                if (err) break;                                      \
            }                                                        \
        }                                                            \
        if (!err) break;                                             \
                                                                     \
        char hostname[STR_HOST_NAME_MAX] = "";                           \
        gethostname(hostname, STR_HOST_NAME_MAX);                        \
        fprintf(stderr, "BINANCE error %s \"%s\" on %s at %s:%d\n",  \
            result["code"].asString().c_str(),                       \
            result["msg"].asString().c_str(),                        \
            hostname, __FILE__, __LINE__);                           \
        if (!getenv("FREEZE_ON_ERROR")) {                            \
            fprintf(stderr, "You may want to set "                   \
                "FREEZE_ON_ERROR environment "                       \
                "variable to debug the case\n");                     \
            exit(-1);                                                \
        }                                                            \
        else {                                                       \
            fprintf(stderr, "thread @ %s "           \
               "is entering infinite loop\n",                        \
               hostname);     \
            while (1) std::this_thread::sleep_for(std::chrono::seconds(1)); /* 1 sec */                   \
        }                                                            \
    } while (0);

#define BINANCE_ERR_CHECK(x)                                         \
    do {                                                             \
                                                                     \
        binanceError_t err = x; if (err != binanceSuccess) {         \
        char hostname[STR_HOST_NAME_MAX] = "";                           \
        gethostname(hostname, STR_HOST_NAME_MAX);                        \
        fprintf(stderr, "BINANCE error %d \"%s\" on %s at %s:%d\n",  \
            (int)err, binanceGetErrorString(err), hostname,          \
            __FILE__, __LINE__);                                     \
        if (!getenv("FREEZE_ON_ERROR")) {                            \
            fprintf(stderr, "You may want to set "                   \
                "FREEZE_ON_ERROR environment "                       \
                "variable to debug the case\n");                     \
            exit(-1);                                                \
        }                                                            \
        else {                                                       \
            fprintf(stderr, "thread  %s "           \
               "is entering infinite loop\n",                        \
               hostname);     \
            while (1) std::this_thread::sleep_for(std::chrono::seconds(1)); /* 1 sec */                   \
        }                                                            \
    }} while (0)

enum binanceError_t
{
    binanceSuccess = 0,
    binanceErrorInvalidServerResponse,
    binanceErrorEmptyServerResponse,
    binanceErrorParsingServerResponse,
    binanceErrorInvalidSymbol,
    binanceErrorMissingAccountKeys,
    binanceErrorCurlFailed,
    binanceErrorCurlOutOfMemory,
    binanceErrorUnknown,
};

struct binanceType
{
    enum type
    {
        binance_cash = 0,
        binance_usdt_fut = 1,
        binance_cb_fut = 2,
    };
};


static const char* binanceGetErrorString(const binanceError_t err)
{
    switch (err)
    {
        BINANCE_CASE_STR(binanceSuccess);
        BINANCE_CASE_STR(binanceErrorInvalidServerResponse);
        BINANCE_CASE_STR(binanceErrorEmptyServerResponse);
        BINANCE_CASE_STR(binanceErrorParsingServerResponse);
        BINANCE_CASE_STR(binanceErrorInvalidSymbol);
        BINANCE_CASE_STR(binanceErrorMissingAccountKeys);
        BINANCE_CASE_STR(binanceErrorCurlFailed);
        BINANCE_CASE_STR(binanceErrorCurlOutOfMemory);
    default:
        // Make compiler happy regarding unhandled enums.
        break;
    }

    static const std::string str_binanceErrorUnknown = "binanceErrorUnknown";
    return str_binanceErrorUnknown.c_str();
}

static void binance_parse_json(Json::Value& json_result, std::string &str_result, binanceError_t &status)
{
	try
	{
		Json::CharReaderBuilder b;
		std::unique_ptr<Json::CharReader> reader(b.newCharReader());
		json_result.clear();
		JSONCPP_STRING errs;
		reader->parse(str_result.data(), str_result.data() + strlen(str_result.data()), &json_result, &errs);
		//CHECK_SERVER_ERR(json_result);
	}
	catch (std::exception& e)
	{
		printf("<get_account> Error ! %s", e.what());
		status = binanceErrorParsingServerResponse;
	}
}
/*interval:1m,3m,5m,15m,30m,1h,2h,4h,6h,8h,12h,1d,3d,1w,1M*/
static bool binance_get_kline(Json::Value& json_result, std::string symbol, binanceType::type tp,
	std::string interval,uint16_t num,std::string &errmsg)
{
	std::string url;
	if (tp == binanceType::binance_usdt_fut){
		url = std::string("https://fapi.binance.com/fapi/v1/klines");
	}
	else if (tp == binanceType::binance_cb_fut)
	{
		url = std::string("https://dapi.binance.com/dapi/v1/klines");
	}
	else
		url = std::string("https://api.binance.com/api/v3/klines");
	
	std::string querystring("?symbol=");
	querystring.append(symbol);

	querystring.append("&interval=");
	querystring.append(interval);

	querystring.append("&limit=");
	querystring.append(std::to_string(num));

	url += querystring;

	std::string str_result;
	int res = getCurl(str_result, url);
	if (res != 0) {
		errmsg = "libcurl get req fail";
		return false;
	}
	
	str_result = "{\"data\":" + str_result + "}";
	binanceError_t status;
	binance_parse_json(json_result, str_result, status);
	if (false == json_result["msg"].empty()) {
		errmsg = json_result["msg"].asString();
		return false;
	}
	return true;
}

class BinanceServer
{
public:
    std::string m_hostname;
    bool m_simulation;
    std::string sessionId;

public:

    BinanceServer(std::string hn = "https://api.binance.com", bool simu = false)
    {
        m_hostname = hn;
        m_simulation = simu;
    }

    const std::string& getHostname() { return m_hostname; }
    bool isSimulator() { return m_simulation; };

    binanceError_t getTime(Json::Value& json_result)
    {
        binanceError_t status = binanceSuccess;

        std::string url(m_hostname);
        url += "/api/v3/time";

        std::string str_result;
        getCurl(str_result, url);

        if (str_result.size() == 0)
            status = binanceErrorEmptyServerResponse;
        else
        {
            try
            {
                Json::CharReaderBuilder b;
                Json::CharReader* reader(b.newCharReader());
                json_result.clear();
                Json::Value json_result;
                JSONCPP_STRING errs;
                reader->parse(str_result.data(), str_result.data() + strlen(str_result.data()), &json_result, &errs);
                CHECK_SERVER_ERR(json_result);
            }
            catch (std::exception& e)
            {
              printf("<get_serverTime> Error ! %s", e.what());
                status = binanceErrorParsingServerResponse;
            }
        }

        return status;
    }
};

class BinanceAccount
{
public:

    BinanceServer m_server;
    std::string m_api_key, m_secret_key, m_listen_key;
    binanceType::type m_type;
    std::shared_ptr<WebSocketSSLClient> wscli;
public:

    BinanceAccount(binanceType::type tp,const std::string api_key = "", const std::string secret_key = "")
    {
        m_type = tp;
        m_api_key = api_key;
        m_secret_key = secret_key;

        if (m_type == binanceType::binance_usdt_fut)
        {
            m_server.m_hostname = std::string("https://fapi.binance.com");
        }
        else if (m_type == binanceType::binance_cb_fut)
        {
            m_server.m_hostname = std::string("https://dapi.binance.com");
        }
        else
            m_server.m_hostname = std::string("https://api.binance.com");
    }

    bool keysAreSet()
    {
        return ((m_api_key.size()==0)|| (m_secret_key.size()==0));
    }

    int send_lws_msg(const char* str,int len)
    {
        //unsigned char buf[LWS_PRE + 10 * 1024];
        //// ǰ��LWS_PRE���ֽڱ�������LWS
        //memset(buf, 0, sizeof(buf));
        //char* msg = (char*)&buf[LWS_PRE];
        //sprintf(msg, "%s", str);
        //// ͨ��WebSocket�����ı���Ϣ
        //return lws_write(conn, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
        std::string strr = std::string(str, len);
        wscli->Send(strr);
        return 0;
    }

    /*void parse_json(Json::Value& json_result, std::string &str_result, binanceError_t &status)
    {
        try
        {
            Json::CharReaderBuilder b;
			std::unique_ptr<Json::CharReader> reader(b.newCharReader());
            json_result.clear();
            JSONCPP_STRING errs;
            reader->parse(str_result.data(), str_result.data() + strlen(str_result.data()), &json_result, &errs);
            //CHECK_SERVER_ERR(json_result);
        }
        catch (std::exception& e)
        {
            printf("<get_account> Error ! %s", e.what());
            status = binanceErrorParsingServerResponse;
        }
    }*/

    binanceError_t getInfo(Json::Value& json_result, long recvWindow = 0)
    {
        binanceError_t status = binanceSuccess;

        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v2/account?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/account?";
            }
            else
                url += "/api/v3/account?";
            std::string action = "GET";

            std::string querystring("timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string post_data = "";

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }
        return status;
    }

    binanceError_t getAccoutBalance(Json::Value& json_result, long recvWindow = 0)
    {
        binanceError_t status = binanceSuccess;

        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);

            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v2/balance?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/balance?";
            }
            else
                url += "/api/v3/account?";
            std::string action = "GET";

            std::string querystring("timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string post_data = "";

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }

            //printf("\n%s\n", str_result.c_str());
        }
        return status;
    }

    binanceError_t getTrades(binanceType::type tp, Json::Value& json_result, const char* symbol, int limit = 500)
    {
        return binanceError_t::binanceSuccess;
    }

    binanceError_t getTradesSigned(Json::Value& json_result, const char* symbol, long fromId = -1,
        long recvWindow = 0, int limit = 500) {
        return binanceError_t::binanceSuccess;
    }

    binanceError_t getHistoricalTrades(Json::Value& json_result, const char* symbol, long fromId = -1, int limit = 500) { return binanceError_t::binanceSuccess; }


    binanceError_t getAllOrders(Json::Value& json_result, const char* symbol,
        long long orderId = 0, long long startTime = 0, long long endTime = 0, int limit = 0, long recvWindow = 0)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/allOrders?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/allOrders?";
            }
            else
            {
                url += "/api/v3/allOrders?";
            }

            std::string querystring("symbol=");
            querystring.append(symbol);

            if (orderId > 0)
            {
                querystring.append("&orderId=");
                querystring.append(std::to_string(orderId));
            }

            if (startTime > 0&& orderId==0)
            {
                querystring.append("&startTime=");
                querystring.append(std::to_string(startTime));
            }

            if (endTime > 0 && orderId == 0)
            {
                querystring.append("&endTime=");
                querystring.append(std::to_string(endTime));
            }

            if (limit > 0)
            {
                querystring.append("&limit=");
                querystring.append(std::to_string(limit));
            }

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            querystring.append("&timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string action = "GET";
            std::string post_data = "";

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }

        return status;
    }

    binanceError_t getAllTrades(Json::Value& json_result, const char* symbol,
        long long fromId = 0, long long startTime = 0, long long endTime = 0, int limit = 0, long recvWindow = 0)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/userTrades?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/userTrades?";
            }
            else
            {
                url += "/api/v3/userTrades?";
            }

            std::string querystring("symbol=");
            querystring.append(symbol);

            if (fromId > 0)
            {
                querystring.append("&fromId=");
                querystring.append(std::to_string(fromId));
            }

            if (startTime > 0 && fromId == 0)
            {
                querystring.append("&startTime=");
                querystring.append(std::to_string(startTime));
            }

            if (endTime > 0 && fromId == 0)
            {
                querystring.append("&endTime=");
                querystring.append(std::to_string(endTime));
            }

            if (limit > 0)
            {
                querystring.append("&limit=");
                querystring.append(std::to_string(limit));
            }

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            querystring.append("&timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string action = "GET";
            std::string post_data = "";

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }

        return status;
    }

    binanceError_t sendOrder(Json::Value& json_result, const char* symbol, const char* side, const char* type,
        const char* timeInForce, double quantity, double price, const char* newClientOrderId, double stopPrice,
        double icebergQty, long recvWindow)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/order?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/order?";
            }
            else
            {
                url += "/api/v3/order?";
            }

            std::string action = "POST";

            std::string post_data("symbol=");
            post_data.append(symbol);

            post_data.append("&side=");
            post_data.append(side);

            post_data.append("&type=");
            post_data.append(type);

            if (strlen(timeInForce) > 0 && strcmp(type,"LIMIT_MAKER")!=0)
            {
                post_data.append("&timeInForce=");
                post_data.append(timeInForce);
            }

            post_data.append("&quantity=");
            post_data.append(toString(quantity));
            
            if (price > 0.0)
            {
                post_data.append("&price=");
                post_data.append(toString(price));
            }

            if (strlen(newClientOrderId) > 0)
            {
                post_data.append("&newClientOrderId=");
                post_data.append(newClientOrderId);
            }

            if (stopPrice > 0.0)
            {
                post_data.append("&stopPrice=");
                post_data.append(toString(stopPrice));
            }

            if (icebergQty > 0.0)
            {
                post_data.append("&icebergQty=");
                post_data.append(toString(icebergQty));
            }

            if (recvWindow > 0)
            {
                post_data.append("&recvWindow=");
                post_data.append(std::to_string(recvWindow));
            }

            post_data.append("&timestamp=");
            post_data.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), post_data.c_str());
            post_data.append("&signature=");
            post_data.append(signature);

            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);
				
			//SPDLOG_INFO("{},{}",url,post_data);
            std::string str_result;
            int res =getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }
        return status;
    }

    binanceError_t sendTestOrder(binanceType::type tp, Json::Value& json_result, const char* symbol, const char* side, const char* type,
        const char* timeInForce, double quantity, double price, const char* newClientOrderId, double stopPrice,
        double icebergQty, long recvWindow)
    {
        return binanceError_t::binanceSuccess;
    }

    binanceError_t getOrder(Json::Value& json_result, const char* symbol,
        const char* orderId, const char* origClientOrderId, long recvWindow)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            if (m_type == binanceType::binance_usdt_fut)
            {
                url = "/fapi/v1/order?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url = "/dapi/v1/order?";
            }
            else
            {
                url += "/api/v3/order?";
            }
            std::string action = "GET";
            std::string querystring("symbol=");
            querystring.append(symbol);

            if (strlen(orderId) > 0)
            {
                querystring.append("&orderId=");
                querystring.append(std::string(orderId));
            }

            if (strlen(origClientOrderId) > 0)
            {
                querystring.append("&origClientOrderId=");
                querystring.append(origClientOrderId);
            }

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            querystring.append("&timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector < std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string str_result;
            std::string post_data = "";
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }
        return status;
    }

    binanceError_t cancelOrder(Json::Value& json_result, const char* symbol,
        const char* orderId, const char* origClientOrderId, const char* newClientOrderId, long recvWindow)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        
        {
            std::string url(m_server.m_hostname);
            
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/order?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/order?";
            }
            else
                url += "/api/v3/order?";

            std::string action = "DELETE";

            std::string post_data("symbol=");
            post_data.append(symbol);

            if (strlen(orderId) > 0)
            {
                post_data.append("&orderId=");
                post_data.append(std::string(orderId));
            }

            if (strlen(origClientOrderId) > 0)
            {
                post_data.append("&origClientOrderId=");
                post_data.append(origClientOrderId);
            }

            if (strlen(newClientOrderId) > 0)
            {
                post_data.append("&newClientOrderId=");
                post_data.append(newClientOrderId);
            }

            if (recvWindow > 0)
            {
                post_data.append("&recvWindow=");
                post_data.append(std::to_string(recvWindow));
            }

            post_data.append("&timestamp=");
            post_data.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), post_data.c_str());
            post_data.append("&signature=");
            post_data.append(signature);

            std::vector < std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }

        return status;
    }

    binanceError_t cancelAllOrder(Json::Value& json_result, const char* symbol)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else

        {
            std::string url(m_server.m_hostname);

            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/allOpenOrders?";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/allOpenOrders?";
            }
            else
                url += "/api/v3/openOrders?";

            std::string action = "DELETE";

            std::string post_data("symbol=");
            post_data.append(symbol);

            post_data.append("&timestamp=");
            post_data.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), post_data.c_str());
            post_data.append("&signature=");
            post_data.append(signature);

            std::vector < std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }

        return status;
    }

    binanceError_t getAccountSummary(Json::Value& json_result, long recvWindow = 0)
    {
        binanceError_t status = binanceSuccess;

        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            url += "/sapi/v2/sub-account/futures/accountSummary?";
         
            std::string action = "GET";

            std::string querystring("timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string post_data = "";

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }
        return status;
    }

    // API key required
    binanceError_t startUserDataStream(Json::Value& json_result)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
           
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/listenKey";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/listenKey";
            }
            else
                url += "/api/v3/userDataStream";

            std::string action = "POST";
            std::string post_data;
            std::vector < std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }
        
        return status;
    }

    binanceError_t keepUserDataStream(const char* listenKey)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
            
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/listenKey";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/listenKey";
            }
            else
                url += "/api/v3/userDataStream";
            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");

            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string action = "PUT";
            std::string post_data("listenKey=");
            post_data.append(listenKey);

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
        }
        return status;
    }


    binanceError_t getOpenOrders(Json::Value& json_result, const char* symbol, long recvWindow = 0) {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0){
            status = binanceErrorMissingAccountKeys;
        }
        else
        {
            std::string url(m_server.m_hostname);
            if (m_type == binanceType::binance_usdt_fut)
            {
                //url = "/fapi/v1/openOrders?";
                printf("not implemented");
                return binanceErrorUnknown;
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                //url = "/dapi/v1/openOrders?";
                printf("not implemented");
                return binanceErrorUnknown;            
            }
            else
            {
                url += "/api/v3/openOrders?";
            }
            std::string action = "GET";
            std::string querystring("symbol=");
            querystring.append(symbol);

            if (recvWindow > 0)
            {
                querystring.append("&recvWindow=");
                querystring.append(std::to_string(recvWindow));
            }

            querystring.append("&timestamp=");
            querystring.append(std::to_string(get_current_ms_epoch()));

            std::string signature = hmac_sha256(m_secret_key.c_str(), querystring.c_str());
            querystring.append("&signature=");
            querystring.append(signature);

            url.append(querystring);
            std::vector < std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");
            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string str_result;
            std::string post_data = "";
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);

            //printf("\n%s\n", str_result.c_str());

            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
            else
            {
				binance_parse_json(json_result, str_result, status);
            }
        }
        return status;
    }

    binanceError_t closeUserDataStream(const char* listenKey)
    {
        binanceError_t status = binanceSuccess;
        if (m_api_key.size() == 0 || m_secret_key.size() == 0)
            status = binanceErrorMissingAccountKeys;
        else
        {
            std::string url(m_server.m_hostname);
           
            if (m_type == binanceType::binance_usdt_fut)
            {
                url += "/fapi/v1/listenKey";
            }
            else if (m_type == binanceType::binance_cb_fut)
            {
                url += "/dapi/v1/listenKey";
            }
            else
                url += "/api/v3/userDataStream";

            std::vector <std::string> extra_http_header;
            std::string header_chunk("X-MBX-APIKEY: ");

            header_chunk.append(m_api_key);
            extra_http_header.push_back(header_chunk);

            std::string action = "DELETE";
            std::string post_data("listenKey=");
            post_data.append(listenKey);

            std::string str_result;
            int res = getCurlWithHeader(str_result, url, extra_http_header, post_data, action);
            if (res != 0)
            {
                return binanceErrorCurlFailed;
            }

            if (str_result.size() == 0)
                status = binanceErrorEmptyServerResponse;
        }
    }
};

#endif
