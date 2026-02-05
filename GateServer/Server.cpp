#include "Server.h"
#include "HttpConnection.h"
#include "IOServicePool.h"

Server::Server(boost::asio::io_context& ioc, unsigned short& port) : _ioc(ioc), 
_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
}

void Server::Start()
{
	auto self = shared_from_this();                                                   // 用来传入lambda表达式，延长Server的生命周期，实现一个伪闭包的效果
	auto& ioc = IOServicePool::GetInstance()->GetIOService();					      // 从IOServicePool单例中获取一个io_context引用，用于异步操作的调度
	std::shared_ptr<HttpConnection> new_connection = std::make_shared<HttpConnection>(ioc);       // 创建一个HttpConnection智能指针，用于管理新连接
	_acceptor.async_accept(new_connection->GetSocket(), [self, new_connection](boost::beast::error_code ec) {     // _acceptor异步接收，接收到连接后将其交给_socket处理并触发回调函数
		try {
			// 若出错则放弃该连接，继续监听其他连接
			if (ec) {
				self->Start();
				return;
			}

			// 没出错则启动该连接
			new_connection->Start();      

			// 继续监听
			self->Start();
		}
		catch (std::exception& e) {
			std::cout << "Server::Start occurred exception. Exception is " << e.what() << std::endl;
			self->Start();
		}
	});
}
