#pragma once
#include <iostream>
#include "CSession.h"
#include <boost/asio/steady_timer.hpp>

// 服务器用于连接的Server类
class Server : public std::enable_shared_from_this<Server>
{
private:
	boost::asio::io_context& _ioc;
	boost::asio::ip::tcp::acceptor _acceptor;							// 服务器监听对象, 用于监听客户端
	short _port;
	std::mutex _mutex;
	boost::asio::steady_timer _timer;									// 定时器对象, 用于定时清理无效Session
	//利用_sessions这个map管理链接，可以增加Session智能指针的引用计数，只有当Session从这个map中移除后，Session才会被释放
	std::map<std::string, std::shared_ptr<Session>> _sessions;			// 通过智能指针的方式管理Session类,key为Session的uid,value为该Session的智能指针
	void StartAccept();
	void HandleAccept(std::shared_ptr<Session> new_session, const boost::system::error_code& ec); // 回调函数
	
public:
	Server(boost::asio::io_context& ioc, short port);
	~Server();
	void ClearSession(std::string session_id);                          // 根据session的id删除session,并移除用户和session的关联
	std::shared_ptr<Session> GetSession(std::string session_id);
	bool CheckValid(std::string session_id);
	//void on_timer(const boost::system::error_code& ec);
	//void StartTimer();
	//void StopTimer();
};

