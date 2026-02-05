#pragma once
#include "const.h"

// 用于管理http连接的类
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(boost::asio::io_context& ioc);
	void Start();
	boost::asio::ip::tcp::socket& GetSocket();                                // 获取socket引用

private:
	void CheckDeadline();                                                     // 检测超时
	void WriteResponse();                                                     // 应答,异步发送HTTP响应给客户端
	void HandleReq();                                                         // 处理请求
	void PreParseGetParam();                                                  // 解析HTTP GET请求URI中的查询参数

	boost::asio::ip::tcp::socket _socket;
	boost::beast::flat_buffer _buffer{ 8192 };                                // 内存缓冲区，专门用于存储网络读写的原始字节数据(比如HTTP请求的原始报文) -> 用于接收数据
	boost::beast::http::request<boost::beast::http::dynamic_body> _request;   // 请求包头,HTTP请求对象，用于存储和解析完整的HTTP请求。模板参数dynamic_body兼顾灵活性，此外还有string_body和file_body -> 用于解析请求
	boost::beast::http::response<boost::beast::http::dynamic_body> _response; // 回应包头,HTTP响应对象，用于构建和发送HTTP响应 -> 用于回应客户端
	boost::asio::steady_timer _deadline{                                      // 用于做定时器判断请求是否超时
		_socket.get_executor(), std::chrono::seconds(60)                      // 每隔60s调度一次,如果http服务器请求超过60s没处理完即超时(注意：steady_timer是"一次性"的，触发后需重新重置才能再次使用)
	};
	std::string _get_url;                                                     // 存储URI的纯路径部分
	std::unordered_map<std::string, std::string> _get_params;                 // 存储解析后的键值对
};

