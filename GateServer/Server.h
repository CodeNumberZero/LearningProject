#pragma once
#include "const.h"

class Server : public std::enable_shared_from_this<Server>
{
public:
	Server(boost::asio::io_context& ioc, unsigned short& port);      
	void Start();                                                    // 启动服务器

private:
	boost::asio::ip::tcp::acceptor _acceptor;
	boost::asio::io_context& _ioc;                                   // io_context没有拷贝构造和拷贝赋值，故使用引用
};

