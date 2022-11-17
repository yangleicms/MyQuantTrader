#pragma once
#ifndef _WEB_SOCKET_CLIENT_H
#define _WEB_SOCKET_CLIENT_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <set>
#include <thread>
#include <chrono>

#include <boost/asio/io_context.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

class WebSocketClientApi
{
public:
	virtual void OnConnected(int id) = 0;
	virtual void OnClosed(std::string uri) = 0;
	virtual void OnReconnect(std::string uri) = 0;

	virtual bool OnSubscribed(char* data, size_t len) {
		return true;
	}

	virtual bool OnUnsubscribed(char* data, size_t len) {
		return true;
	}

	virtual bool OnDataReady(char* data, size_t len, int id) {
		return true;
	}

	virtual std::string GenerateSubscribeData(std::unordered_set<std::string> pairs) {
		return "";
	}

	virtual std::string GenerateUnsubscribeData(std::unordered_set<std::string> pairs) {
		return"";
	}
};

template<typename WSType>
class WebSocketClientBase :public std::enable_shared_from_this<WebSocketClientBase<WSType>> {
public:
	WebSocketClientBase(int id, boost::asio::io_context& ioctx, const std::string& host, const std::string& port, WebSocketClientApi* api) :
		id_(id), ioctx_(ioctx), resolver_{ ioctx_ }, host_(host), port_(port), api_(api)
	{}

	~WebSocketClientBase() {
		Close();
	}

	virtual void Connect(const std::string& uri) = 0;

	virtual void Close() {
		try {
			stop_ = true;
			ws_->close(boost::beast::websocket::close_code::normal);
		}
		catch (boost::exception const& ignored) {
		}
	}

	virtual void Reconnect() {
		std::string msg = uri_ + " ready to Reconn";
		api_->OnReconnect(msg);
		if (!stop_) {
			msg = uri_ + " has been stop,ready to close";
			api_->OnReconnect(msg);
			Close();
		}
		//std::this_thread::sleep_for(std::chrono::seconds(3));
		std::this_thread::sleep_for(std::chrono::seconds(2));
		msg = uri_ + " exec Reconning....";
		api_->OnReconnect(msg);
		Connect(uri_);
	}

	void Subscribe(std::unordered_set<std::string> pairs) {
		pairs_ = pairs;
		auto subdata = api_->GenerateSubscribeData(pairs);
		ws_->async_write(boost::asio::buffer(subdata),
			std::bind(&WebSocketClientBase::OnSendSubscribeData,
				this->shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void Unsubscribe(std::unordered_set<std::string> pairs) {
		auto unsubdata = api_->GenerateUnsubscribeData(pairs);
		ws_->async_write(boost::asio::buffer(unsubdata),
			std::bind(&WebSocketClientBase::OnSendUnsubscribeData,
				this->shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void StartRead() {
		AsyncReadData();
	}

	void Send(const std::string& data) {
		ws_->async_write(boost::asio::buffer(data),
			std::bind(&WebSocketClientBase::OnRead,
				this->shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

protected:
	virtual void OnResolved(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type res) = 0;

	virtual void OnConnected(boost::beast::error_code ec) = 0;

	void OnReady(boost::beast::error_code ec) {
		if (ec) {
			printf("[%d] fail to process handshake for host %s,err message:%s\n", id_, host_.c_str(), ec.message().c_str());
			Reconnect();
			return;
		}
		printf("setup websocket connection to %s:%s succeeded\n", host_.c_str(), port_.c_str());
		api_->OnConnected(id_);
		StartRead();
	}

	void OnSendSubscribeData(boost::system::error_code ec, std::size_t bytes_transferred) {
		if (ec) {
			printf("fail to subsCribe data\n");
			if (!stop_) {
				Reconnect();
			}
			return;
		}

		ws_->async_read(
			buffer_,
			std::bind(
				&WebSocketClientBase::OnSubscribeRetMsg,
				this->shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void OnSubscribeRetMsg(boost::beast::error_code ec, std::size_t recv_len) {
		if (ec) {
			printf("fail to recv subscribe return message\n");
			if (!stop_) {
				Reconnect();
				return;
			}
		}

		auto size = buffer_.size();
		if (size != recv_len) {
			printf("recv len error\n");
			return;
		}
		char buf[size] = { 0 };
		int total_len = 0;
		for (const auto& it : buffer_.data()) {
			auto data_size = it.size();
			memcpy(buf + total_len, static_cast<const char*>(it.data()), data_size);
			total_len += data_size;
		}
		buffer_.consume(buffer_.size());
		bool ok = api_->OnSubscribed(buf, total_len);
		if (!ok) {
			printf("process data failed\n");
		}
		else {
			AsyncReadData();
		}
	}

	void OnSendUnsubscribeData(boost::system::error_code ec, std::size_t bytes_transferred) {
		if (ec) {
			printf("fail to subscribe data\n");
			if (!stop_) {
				Reconnect();
			}
			return;
		}

		ws_->async_read(buffer_,
			std::bind(
				&WebSocketClientBase::OnUnsubscribeRetMsg,
				this->shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void OnUnsubscribeRetMsg(boost::beast::error_code ec, std::size_t recv_len) {
		if (ec) {
			printf("fail to recv subscribe return message\n");
			if (!stop_) {
				Reconnect();
				return;
			}
		}

		auto size = buffer_.size();
		if (size != recv_len) {
			printf("recv len error\n");
			return;
		}

		char buf[size] = { 0 };
		int total_len = 0;
		for (const auto& it : buffer_.data()) {
			auto data_size = it.size();
			memcpy(buf + total_len, static_cast<const char*>(it.data()), data_size);
			total_len += data_size;
		}
		buffer_.consume(buffer_.size());

		bool ok = api_->OnUnsubscribed(buf, total_len);
		if (!ok) {
			printf("process data failed\n");
		}
		else {
			;
		}
	}

	void AsyncReadData() {
		ws_->async_read(buffer_, std::bind(&WebSocketClientBase::OnRead, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}

	void OnRead(boost::beast::error_code ec, std::size_t recv_len) {
		if (ec) {
			printf("[%d]:%s fail to read data,err message:%s\n", id_, host_.c_str(), ec.message().c_str());
			if (!stop_) {
				Reconnect();
			}
			return;
		}

		auto size = buffer_.size();
		if (size != recv_len) {
			//printf("recv len error:buf:%d,rcv:%d\n",(int)size,(int)recv_len);
			return;
		}

		//char buf[size + 1] = { 0 };
		//printf("%d,%d\n",(int)size,(int)recv_len);
#ifdef _WIN32
		char* buf = (char*)malloc(size + 1);
#else
		char buf[size + 1] = { 0 };
#endif
		int total_len = 0;
		for (const auto& it : buffer_.data()) {
			auto data_size = it.size();
			memcpy(buf + total_len, static_cast<const char*>(it.data()), data_size);
			total_len += data_size;
		}
		buffer_.consume(buffer_.size());

		bool ok = api_->OnDataReady(buf, total_len, id_);
		if (!ok) {
			printf("process data failed\n");
		}
		if (!stop_) {
			AsyncReadData();
		}
#ifdef _WIN32
		free(buf);
#endif
	}

	void OnClose(boost::system::error_code ec) {
		if (ec) {
			printf("fail to close ws,%s\n", ec.message().c_str());
		}
		api_->OnClosed(uri_);
		stop_ = true;
	}

	template <typename Derived>
	std::shared_ptr<Derived> shared_from_base()
	{
		return std::static_pointer_cast<Derived>(this->shared_from_this());
	}

	int id_;
	boost::asio::io_context& ioctx_;
	boost::asio::ip::tcp::resolver resolver_;
	std::shared_ptr<boost::beast::websocket::stream<WSType>>ws_;
	std::string host_;
	std::string port_;
	std::string uri_;
	std::set<std::string> pairs_;
	WebSocketClientApi* api_;
	boost::beast::multi_buffer buffer_;
	bool stop_ = true;
};

class WebSocketSSLClient :public WebSocketClientBase<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> {
public:
	WebSocketSSLClient(int id, boost::asio::io_context& ioctx,
		const std::string& host, const std::string& port, WebSocketClientApi* api) :
		WebSocketClientBase(id, ioctx, host, port, api),
		ssl_{ boost::asio::ssl::context::sslv23_client }
	{
		ws_ = std::make_shared<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(ioctx_, ssl_);
	}
	~WebSocketSSLClient() {
		Close();
	}

	void Connect(const std::string& uri) override {
		uri_ = uri.empty() ? "/" : uri;
		resolver_.async_resolve(host_, port_,
			std::bind(&WebSocketSSLClient::OnResolved, shared_from_base<WebSocketSSLClient>(),
				std::placeholders::_1, std::placeholders::_2));
	}

	void Reconnect() override {
		ws_.reset(new boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(ioctx_, ssl_));
		WebSocketClientBase::Reconnect();
	}

private:

	void OnResolved(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type res)override {
		if (ec) {
			printf("[%d] fail to resolve host %s,err message:%s\n", id_, host_.c_str(), ec.message().c_str());
			Reconnect();
			return;
		}

		if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host_.c_str())) {
			printf("failed to set host name\n");
			Reconnect();
			return;
		}

		boost::asio::async_connect(ws_->next_layer().next_layer(), res.begin(), res.end(),
			std::bind(&WebSocketSSLClient::OnConnected, shared_from_base<WebSocketSSLClient>(), std::placeholders::_1));
	}

	void OnConnected(boost::beast::error_code ec)override {
		if (ec) {
			printf("[%d]fail to connect to host %s,err message:%s\n", id_, host_.c_str(), ec.message().c_str());
			Reconnect();
			return;
		}
#ifndef _WIN32
		ws_->control_callback(
			[this](boost::beast::websocket::frame_type kind, boost::beast::string_view payload) {
				ws_->async_pong(
					boost::beast::websocket::ping_data{}, [](boost::beast::error_code ec) {}
				);
			}
		);
#endif
		stop_ = false;
		ws_->next_layer().async_handshake(
			boost::asio::ssl::stream_base::client,
			std::bind(&WebSocketSSLClient::OnSSLHandshake, shared_from_base<WebSocketSSLClient>(), std::placeholders::_1)
		);
	}

	void OnSSLHandshake(boost::beast::error_code ec) {
		ws_->async_handshake(host_, uri_, std::bind(&WebSocketSSLClient::OnReady,
			shared_from_base<WebSocketSSLClient>(), std::placeholders::_1));
	}

private:
	boost::asio::ssl::context ssl_;

};

#endif
