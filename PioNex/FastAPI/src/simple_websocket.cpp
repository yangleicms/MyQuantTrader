#include "simple_websocket.h"
#include "binacpp_logger.h"

int simple_websocket::event_cb( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{

	switch( reason )
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			lws_callback_on_writable( wsi );
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			
			/* Handle incomming messages here. */
			try {

				//BinaCPP_logger::write_log("%p %s",  wsi, (char *)in );

				string str_result = string( (char*)in );
				Json::Reader reader;
				Json::Value json_result;	
				reader.parse( str_result , json_result );

				if (single_con::get_instance()->handles.find( wsi ) != single_con::get_instance()->handles.end() ) {
					single_con::get_instance()->handles[wsi]( json_result );
				}

			} catch ( exception &e ) {
		 		BinaCPP_logger::write_log( "<simple_websocket::event_cb> Error ! %s", e.what() ); 
			}   	
			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
			break;
		}

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			if (single_con::get_instance()->handles.find( wsi ) != single_con::get_instance()->handles.end() ) {
				single_con::get_instance()->handles.erase(wsi);
			}
			break;

		default:
			break;
	}

	return 0;
}

//-------------------
void simple_websocket::init( ) 
{
	struct lws_context_creation_info info;
	memset( &info, 0, sizeof(info) );

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

	single_con::get_instance()->context = lws_create_context( &info );
}

//----------------------------
// Register call backs
void simple_websocket::connect_endpoint (CB cb,const char *path) 
{
	char ws_path[1024];
	strcpy( ws_path, path );
	
	
	/* Connect if we are not connected to the server. */
	struct lws_client_connect_info ccinfo = {0};
	ccinfo.context 	= single_con::get_instance()->context;
	ccinfo.address 	= BINANCE_WS_HOST;
	ccinfo.port 	= BINANCE_WS_PORT;
	ccinfo.path 	= ws_path;
	ccinfo.host 	= lws_canonical_hostname(single_con::get_instance()->context );
	ccinfo.origin 	= "origin";
	ccinfo.protocol = protocols[0].name;
	ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

	struct lws* conn = lws_client_connect_via_info(&ccinfo);
	single_con::get_instance()->handles[conn] = cb;
}

void simple_websocket::connect_pionex_public(CB cb)
{
	char ws_path[1024];
	//strcpy(ws_path, path);

	/* Connect if we are not connected to the server. */
	struct lws_client_connect_info ccinfo = { 0 };
	ccinfo.context = single_con::get_instance()->context;
	ccinfo.address = PIONEX_PUBLIC_WS;
	ccinfo.port =  WS_PORT;
	//ccinfo.path = ws_path;
	ccinfo.host = lws_canonical_hostname(single_con::get_instance()->context);
	ccinfo.origin = "origin";
	ccinfo.protocol = protocols[0].name;
	ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

	struct lws* conn = lws_client_connect_via_info(&ccinfo);
	single_con::get_instance()->handles[conn] = cb;
	single_con::get_instance()->pionex_pub_conn = conn;
	if(nullptr == single_con::get_instance()->pionex_pub_conn)
		std::cout<<"nullptr == single_con::get_instance()->pionex_pub_conn\n";
}

void simple_websocket::connect_pionex_private(CB cb, const char* path)
{
	char ws_path[1024];
	strcpy(ws_path, path);


	/* Connect if we are not connected to the server. */
	struct lws_client_connect_info ccinfo = { 0 };
	ccinfo.context = single_con::get_instance()->context;
	ccinfo.address = BINANCE_WS_HOST;
	ccinfo.port = BINANCE_WS_PORT;
	ccinfo.path = ws_path;
	ccinfo.host = lws_canonical_hostname(ingle_con::get_instance()->context);
	ccinfo.origin = "origin";
	ccinfo.protocol = protocols[0].name;
	ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

	struct lws* conn = lws_client_connect_via_info(&ccinfo);
	single_con::get_instance()->handles[conn] = cb;
}

int simple_websocket::send_pionex_lws_msg(const char* str, int len)
{
	unsigned char buf[LWS_PRE + 10 * 1024];
	// 前面LWS_PRE个字节必须留给LWS
	memset(buf, 0, sizeof(buf));
	char* msg = (char*)&buf[LWS_PRE];
	sprintf(msg, "%s", str);
	// 通过WebSocket发送文本消息
	if(single_con::get_instance()->pionex_pub_conn==nullptr)
		std::cout<<"pionex_pub_conn NULL\n";
	return lws_write(single_con::get_instance()->pionex_pub_conn, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
}

//----------------------------
// Entering event loop
void simple_websocket::enter_event_loop() 
{
	while( 1 )
	{	
		try {	
			lws_service(single_con::get_instance()->context, 500 );
		} catch ( exception &e ) {
		 	BinaCPP_logger::write_log( "<simple_websocket::enter_event_loop> Error ! %s", e.what() ); 
		 	break;
		}
	}
	lws_context_destroy(single_con::get_instance()->context );
}


