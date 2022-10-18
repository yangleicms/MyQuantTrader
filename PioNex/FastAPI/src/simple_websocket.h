


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <exception>

#include <json/json.h>
#include <libwebsockets.h>


#define BINANCE_WS_HOST "stream.binance.com"
#define BINANCE_WS_PORT 9443

#define WS_PORT 443
#define PIONEX_PRIVATE_WS "ws.pionex.com"
#define PIONEX_PUBLIC_WS "ws.pionex.com"

using namespace std;
typedef int (*CB)(Json::Value &json_value );

class single_con
{
public:
	static single_con* get_instance()
	{
		static single_con res;
		return &res;
	}
	int i;
	lws_context* context = nullptr;
	lws* pionex_pub_conn=nullptr;
	std::map<lws*, CB> handles;

private:
	single_con() {}
	~single_con() {}
};

class simple_websocket {
	public:
		static struct lws_protocols protocols[];
		static int  event_cb( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len );
		static void connect_endpoint(CB user_cb,const char* path);
		static void init();
		static void enter_event_loop();

		static int send_pionex_lws_msg(const char* str, int len);
		static void connect_pionex_public(CB user_cb);
		static void connect_pionex_private(CB user_cb, const char* path);
		
};

