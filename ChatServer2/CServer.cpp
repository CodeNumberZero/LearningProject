#include "CServer.h"
#include "IOServicePool.h"
#include "ConfigMgr.h"
#include "UserMgr.h"

Server::Server(boost::asio::io_context& ioc, short port) 
	: _ioc(ioc), _port(port), _acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)), _timer(ioc, std::chrono::seconds(60))
{
	std::cout << "ChatServer start success, on port " << port << std::endl;
	StartAccept();                                                                       // 构造Server后启动监听
}

Server::~Server()
{
	std::cout << "ChatServer destruct listen on port : " << _port << std::endl;
}

void Server::StartAccept() {
	auto& ios = IOServicePool::GetInstance()->GetIOService();
	//虽然new_session是一个局部变量，但是通过bind操作，将new_session作为数值传递给bind函数，而bind函数返回的函数对象内部引用了该new_session所以引用计数增加1，这样保证了new_session不会被释放(换成lambda表达式可能更好理解)
	std::shared_ptr<Session> new_session = std::make_shared<Session>(ios, this);         // 启动监听时先创建一个Session类,从池子中获取一个io_context对象用于构造Session类
	// 传递给Server的ioc负责连接的分发(即与_acceptor关联的ioc),IOServicePool中的ioc负责事件的处理(即每个Session关联的ioc),类似muduo库中的主从reactor模式;GateServer也使用了这种做法
	_acceptor.async_accept(new_session->GetSocket(), 
		std::bind(&Server::HandleAccept, this, new_session, std::placeholders::_1));     // 服务器监听到连接后，后续该连接都交给刚刚创建的Session类处理
	/*
		一、绑定类成员函数需要对象指针或引用,因此第二个参数传this指针
		二、使用了一个占位符留给错误码。在boost::asio内部大致流程是：
			1、首先std::bind创建了一个函数对象func,这个对象已经存储了this指针和new_session智能指针,预留了一个位置给error_code
			2、之后asio发起异步操作async_accept,当操作完成时，操作系统会返回错误码error_code
			3、这时asio就会调用func,并传入error_code,完成回调函数的调用
	*/
}

void Server::HandleAccept(std::shared_ptr<Session> new_session, const boost::system::error_code& ec) {
	if (!ec) {
		new_session->Start();                 // 如果连接没有报错，Session开始处理客户端的消息
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.insert(std::make_pair(new_session->GetSessionId(), new_session)); // 将成功连接的会话添加到map中进行管理
	}
	else {
		std::cout << "session accept failed, error is " << ec.what() << std::endl;
	}

	StartAccept();                           // 处理完一个连接后，acceptor继续接收新的连接
}

void Server::ClearSession(std::string session_id) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		auto uid = _sessions[session_id]->GetUserId();
		// 移除用户和session的关联
		UserMgr::GetInstance()->RemoveUserSession(uid);
	}
	_sessions.erase(session_id);
}

std::shared_ptr<Session> Server::GetSession(std::string session_id)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(session_id);
	if (it != _sessions.end()) {
		return it->second;
	}
	return nullptr;
}

bool Server::CheckValid(std::string session_id)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(session_id);
	if (it != _sessions.end()) {
		return true;
	}
	return false;
}

//void Server::on_timer(const boost::system::error_code& ec)
//{
//	if (ec) {
//		std::cout << "timer error: " << ec.message() << std::endl;
//		return;
//	}
//	std::vector<std::shared_ptr<Session>> _expired_sessions;
//	int session_count = 0;
//	//此处加锁遍历session
//	std::map<std::string, std::shared_ptr<Session>> sessions_copy;
//	{
//		std::lock_guard<std::mutex> lock(_mutex);
//		sessions_copy = _sessions;
//	}
//
//	time_t now = std::time(nullptr);
//	for (auto iter = sessions_copy.begin(); iter != sessions_copy.end(); iter++) {
//		auto b_expired = iter->second->IsHeartbeatExpired(now);
//		if (b_expired) {
//			//关闭socket, 其实这里也会触发async_read的错误处理
//			iter->second->Close();
//			//收集过期信息
//			_expired_sessions.push_back(iter->second);
//			continue;
//		}
//		session_count++;
//	}
//
//	//设置session数量
//	auto& cfg = ConfigMgr::GetInstance();
//	auto self_name = cfg["SelfServer"]["Name"];
//	auto count_str = std::to_string(session_count);
//	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);
//
//	//处理过期session, 单独提出，防止死锁
//	for (auto& session : _expired_sessions) {
//		session->DealExceptionSession();
//	}
//
//	//再次设置，下一个60s检测
//	_timer.expires_after(std::chrono::seconds(60));
//	_timer.async_wait([this](boost::system::error_code ec) {
//		on_timer(ec);
//	});
//}

//void Server::StartTimer()
//{
//	auto self(shared_from_this());
//	_timer.async_wait([self](boost::system::error_code ec) {
//		self->on_timer(er);
//	});
//}

//void Server::StopTimer()
//{
//	_timer.cancel();
//}
