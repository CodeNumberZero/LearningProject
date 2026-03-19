#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"

Session::Session(boost::asio::io_context& ioc, Server* server) : _socket(ioc), _server(server), _b_close(false), _b_head_parse(false), _user_uid(0) {
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();       // 第一个括号是构建临时对象，第二个括号是调用重载的()运算符，即仿函数
	_session_id = boost::uuids::to_string(a_uuid);
	_rece_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LENGTH);       // 初始化接收头部消息的节点
	_last_heartbeat = std::time(nullptr);                                 // 参数为 nullptr 时表明函数忽略参数，返回当前时间的 time_t 值
}

Session::~Session() {
	std::cout << "Session destruct delete this:" << this << std::endl;
}

boost::asio::ip::tcp::socket& Session::GetSocket() {
	return _socket;
}

const std::string& Session::GetSessionId() const {
	return _session_id;
}

void Session::SetUserId(int uid)
{
	_user_uid = uid;
}

int Session::GetUserId()
{
	return _user_uid;
}

void Session::Start() {
	AsyncReadHead(HEAD_TOTAL_LENGTH);
}

// 发送队列的作用：1、保证消息异步发送的有序性 2、解耦
void Session::Send(char* msg, short max_length, short msg_id) {
	std::lock_guard<std::mutex> lock(_send_mtx);                             // 给消息队列上锁，保证队列的安全性
	int queue_size = _send_queue.size();
	if (queue_size > MAX_SEND_QUE_SIZE) {
		std::cout << "session: " << _session_id << " send queue fulled, max size is " << MAX_SEND_QUE_SIZE << std::endl;
		return;
	}

	_send_queue.push(std::make_shared<SendNode>(msg, max_length, msg_id));             // 将当前的消息加入队列
	if (queue_size > 0) {                                                     
		return;
	}

	auto& msgnode = _send_queue.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),   // 队列中只有当前消息，直接发送出去
			std::bind(&Session::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

// 发送队列的作用：1、保证消息异步发送的有序性 2、解耦
void Session::Send(std::string msg, short msg_id) {
	std::lock_guard<std::mutex> lock(_send_mtx);
	int queue_size = _send_queue.size();
	if (queue_size > MAX_SEND_QUE_SIZE) {
		std::cout << "session: " << _session_id << " send queue fulled, max size is " << MAX_SEND_QUE_SIZE << std::endl;
		return;
	}

	_send_queue.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msg_id));             // 将当前的消息加入队列
	if (queue_size > 0) {
		return;
	}

	auto& msgnode = _send_queue.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),   // 队列中只有当前消息，直接发送出去
			std::bind(&Session::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void Session::Close() {
	std::lock_guard<std::mutex> lock(_session_mtx);
	_socket.close();
	_b_close = true;
}

std::shared_ptr<Session> Session::SharedSelf() {
	return shared_from_this();
}

void Session::AsyncReadHead(int total_len)
{
	auto self = shared_from_this();
	// 封装的asyncReadFull函数保证读取HEAD_TOTAL_LENGTH个字节后再触发回调
	asyncReadFull(HEAD_TOTAL_LENGTH, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "AsyncReadHead handle read failed, error is " << ec.what() << std::endl;
				Close();
				//DealExceptionSession();
				_server->ClearSession(_session_id);
				return;
			}

			if (bytes_transfered < HEAD_TOTAL_LENGTH) {
				std::cout << "AsyncReadHead read length not match, read [" << bytes_transfered << "] , total ["
					<< HEAD_TOTAL_LENGTH << "]" << std::endl;
				Close();
				_server->ClearSession(_session_id);
				return;
			}

			//判断连接无效
			if (!_server->CheckValid(_session_id)) {
				Close();
				return;
			}

			_rece_head_node->Clear();
			memcpy(_rece_head_node->_data, _data, bytes_transfered);

			//获取头部MSGID数据
			short msg_id = 0;
			memcpy(&msg_id, _rece_head_node->_data, HEAD_ID_LENGTH);
			//网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << std::endl;
			//id非法
			if (msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << std::endl;
				_server->ClearSession(_session_id);
				return;
			}
			short msg_len = 0;
			memcpy(&msg_len, _rece_head_node->_data + HEAD_ID_LENGTH, HEAD_DATA_LENGTH);
			//网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is " << msg_len << std::endl;

			//id非法
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << std::endl;
				_server->ClearSession(_session_id);
				return;
			}

			_rece_msg_node = std::make_shared<ReceNode>(msg_len, msg_id);
			AsyncReadBody(msg_len);
		}
		catch (std::exception& e) {
			std::cout << "AsyncReadHead's Exception code is " << e.what() << std::endl;
		}
		});
}

void Session::AsyncReadBody(int total_len)
{
	auto self = shared_from_this();
	asyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "AsyncReadBody handle read failed, error is " << ec.what() << std::endl;
				Close();
				//DealExceptionSession();
				_server->ClearSession(_session_id);
				return;
			}

			if (bytes_transfered < total_len) {
				std::cout << "AsyncReadBody read length not match, read [" << bytes_transfered << "] , total ["
					<< total_len << "]" << std::endl;
				Close();
				_server->ClearSession(_session_id);
				return;
			}

			//判断连接无效
			if (!_server->CheckValid(_session_id)) {
				Close();
				return;
			}

			memcpy(_rece_msg_node->_data, _data, bytes_transfered);
			_rece_msg_node->_cur_len += bytes_transfered;
			_rece_msg_node->_data[_rece_msg_node->_total_len] = '\0';
			std::cout << "receive data is " << _rece_msg_node->_data << std::endl;
			//更新session心跳时间
			//UpdateHeartbeat();
			//此处将消息投递到逻辑队列中
			LogicSystem::GetInstance()->PostMsgToQueue(std::make_shared<LogicNode>(shared_from_this(), _rece_msg_node));
			//继续监听头部接受事件
			AsyncReadHead(HEAD_TOTAL_LENGTH);
		}
		catch (std::exception& e) {
			std::cout << "AsyncReadBody's Exception code is " << e.what() << std::endl;
		}
	});
}

void Session::NotifyOffline(int uid)
{
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;

	std::string return_str = rtvalue.toStyledString();

	Send(return_str, ID_NOTIFY_OFF_LINE_REQ);
	return;
}

bool Session::IsHeartbeatExpired(std::time_t& now)
{
	double diff_sec = std::difftime(now, _last_heartbeat);          // 用于计算两个时间之间差异的函数
	if (diff_sec > 20) {
		std::cout << "heartbeat expired, session id is  " << _session_id << std::endl;
		return true;
	}
	return false;
}

void Session::UpdateHeartbeat()
{
	time_t now = std::time(nullptr);
	_last_heartbeat = now;
}

//void Session::DealExceptionSession()
//{
//	auto self = shared_from_this();
//	//加锁清除session
//	auto uid_str = std::to_string(_user_uid);
//	auto lock_key = LOCK_PREFIX + uid_str;
//	auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
//	Defer defer([identifier, lock_key, self, this]() {
//		_server->ClearSession(_session_id);
//		RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
//	});
//
//	if (identifier.empty()) {
//		return;
//	}
//	std::string redis_session_id = "";
//	auto bsuccess = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id);
//	if (!bsuccess) {
//		return;
//	}
//
//	if (redis_session_id != _session_id) {
//		//说明有客户在其他服务器异地登录了
//		return;
//	}
//
//	RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
//	//清除用户登录信息
//	RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
//}

// 全双工通信写法
void Session::HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<Session> shared_self) {
	try {
		if (!ec) {
			int copy_len = 0;                                                            // 已经移动的字符数
			while (bytes_transferred > 0) {                                              // 大于0说明读取到了数据
				if (!_b_head_parse) {                                                      // 头部未解析完
					if (bytes_transferred + _rece_head_node->_cur_len < HEAD_TOTAL_LENGTH) {   // 收到的数据不足头部大小
						memcpy(_rece_head_node->_data + _rece_head_node->_cur_len, _data + copy_len, bytes_transferred);
						_rece_head_node->_cur_len += bytes_transferred;
						memset(_data, 0, MAX_LENGTH);
						_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
								std::bind(&Session::HandleRead, this, std::placeholders::_1, std::placeholders::_2, shared_self));
						return;
					}
					// 收到的数据比头部多
					int head_remain = HEAD_TOTAL_LENGTH - _rece_head_node->_cur_len;           // 头部剩余未复制的长度
					memcpy(_rece_head_node->_data + _rece_head_node->_cur_len, _data + copy_len, head_remain);
					copy_len += head_remain;                                             // 更新已处理的data长度
					bytes_transferred -= head_remain;                                    // 更新剩余未处理的长度
					
					// 获取头部消息id的数据
					short msg_id = 0;                                                    
					memcpy(&msg_id, _rece_head_node->_data, HEAD_ID_LENGTH);
					msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id); // 网络字节序转为本机字节序
					std::cout << "msg id is " << msg_id << std::endl;
					if (msg_id > MAX_LENGTH) {
						std::cout << "Invalid msg id! msg id id " << msg_id << std::endl;
						_server->ClearSession(_session_id);
						return;
					}

					// 获取头部消息长度的数据
					short msg_len = 0;
					memcpy(&msg_len, _rece_head_node->_data + HEAD_ID_LENGTH, HEAD_DATA_LENGTH);              
					msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len); // 将网络字节序转化为本机字节序
					std::cout << "msg_len is " << msg_len << std::endl;
					if (msg_len > MAX_LENGTH) {                                         // 如果头部数据非法
						std::cout << "Data length is too long! invalid data length is " << msg_len << std::endl;
						_server->ClearSession(_session_id);                                    // 清除该Session
						return;
					}
					_rece_msg_node = std::make_shared<ReceNode>(msg_len, msg_id);        // 初始化接收消息内容的节点

					if (bytes_transferred < msg_len) {                                  // 如果消息的长度小于头部规定的长度，说明数据未收全，则先将部分消息放到接收节点里
						memcpy(_rece_msg_node->_data + _rece_msg_node->_cur_len, _data + copy_len, bytes_transferred);
						_rece_msg_node->_cur_len += bytes_transferred;                   // 更新已接收的消息长度
						memset(_data, 0, MAX_LENGTH);                                    // 清空，用于下次接收数据
						_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),  // 开始监听下一次读事件
								std::bind(&Session::HandleRead, this, std::placeholders::_1, std::placeholders::_2, shared_self));
						_b_head_parse = true;                                              // 改变标志位，表示头部已处理完
						return;
					}

					// 如果消息的长度不小于头部规定的长度，说明该条数据收全，将消息放到接收节点里
					memcpy(_rece_msg_node->_data + _rece_msg_node->_cur_len, _data + copy_len, msg_len);
					_rece_msg_node->_cur_len += msg_len;
					copy_len += msg_len;
					bytes_transferred -= msg_len;
					_rece_msg_node->_data[_rece_msg_node->_total_len] = '\0';

					LogicSystem::GetInstance()->PostMsgToQueue(std::make_shared<LogicNode>(shared_self, _rece_msg_node)); // 将消息节点投到逻辑层进行逻辑处理

					// 继续轮询剩余未处理数据(也就是接收到的下一条数据)
					_b_head_parse = false;
					_rece_head_node->Clear();                                            // 清空消息节点，重复使用，减少构造开销
					if (bytes_transferred <= 0) {                                        // 如果处理完上一条消息后，未处理数据长度<=0,说明该次读取的数据已处理完，此时清空数据域，继续监听下一个读事件
						memset(_data, 0, MAX_LENGTH);
						_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
								std::bind(&Session::HandleRead, this, std::placeholders::_1, std::placeholders::_2,shared_self));
						return;
					}
					continue;
				}

				// 已经处理完头部，处理上次未接受完的消息数据
				int remain_msg = _rece_msg_node->_total_len - _rece_msg_node->_cur_len;
				if (bytes_transferred < remain_msg) {                                    // 如果接收的数据仍不足剩余未处理的
					memcpy(_rece_msg_node->_data + _rece_msg_node->_cur_len, _data + copy_len, bytes_transferred);
					_rece_msg_node->_cur_len += bytes_transferred;
					memset(_data, 0, MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
							std::bind(&Session::HandleRead, this, std::placeholders::_1, std::placeholders::_2, shared_self));
					return;
				}
				memcpy(_rece_msg_node->_data + _rece_msg_node->_cur_len, _data + copy_len, remain_msg);
				bytes_transferred -= remain_msg;
				copy_len += remain_msg;
				_rece_msg_node->_data[_rece_msg_node->_total_len] = '\0';

				LogicSystem::GetInstance()->PostMsgToQueue(std::make_shared<LogicNode>(shared_self, _rece_msg_node)); // 将消息节点投到逻辑层进行逻辑处理

				// 继续轮询剩余未处理数据(和前面类似)
				_b_head_parse = false;
				_rece_head_node->Clear();                                            // 清空消息节点，重复使用，减少构造开销
				if (bytes_transferred <= 0) {                                        // 如果处理完上一条消息后，未处理数据长度<=0,说明该次读取的数据已处理完，此时清空数据域，继续监听下一个读事件
					memset(_data, 0, MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
							std::bind(&Session::HandleRead, this, std::placeholders::_1, std::placeholders::_2, shared_self));
					return;
				}
				continue;
			}
		}
		else {
			std::cout << "Read error! error code = " << ec.value() << ". Message is " << ec.message() << std::endl;
			Close();
			_server->ClearSession(_session_id);                                          // 读取失败则移除该Session类
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code = " << e.what() << std::endl;
	}
}

// 全双工通信写法
void Session::HandleWrite(const boost::system::error_code& ec, std::shared_ptr<Session> shared_self) {
	try {                                                                    // 增加异常处理
		auto self = shared_from_this();
		if (!ec) {
			std::lock_guard<std::mutex> lock(_send_mtx);                     // 先上锁
			_send_queue.pop();                                               // 当触发该回调函数时说明上一条消息已发送完(因为用的是async_write)，弹出队列首元素
			if (!_send_queue.empty()) {                                      // 队列不为空取队首元素继续发送消息
				auto& msgnode = _send_queue.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
						std::bind(&Session::HandleWrite, this, std::placeholders::_1, shared_self));
			}
		}
		else {
			std::cout << "Handle write failed! error code = " << ec.value() << ". Message is " << ec.message() << std::endl;
			Close();
			//DealExceptionSession();
			_server->ClearSession(_session_id);
		}
	}
	catch (std::exception& e) {
		std::cerr << "HandleWrite's Exception code = " << e.what() << std::endl;
	}
}

// 读取完整长度
void Session::asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	::memset(_data, 0, MAX_LENGTH);
	asyncReadLen(0, maxLength, handler);
}

// 读取指定字节数
// 参数read_len表示已经读取的字节数,下一次读取从第read_len+1个字节开始读,即下一次读取时_data的偏移位置
// 参数total_len表示要读取的总字节数
void Session::asyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	auto self = shared_from_this();
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
		[read_len, total_len, handler, self](const boost::system::error_code& ec, std::size_t  bytesTransfered) {
			if (ec) {
				// 出现错误，调用回调函数
				handler(ec, read_len + bytesTransfered);          // read_len + bytesTransfered表示之前已经读取的长度和本次读取的长度之和
				return;
			}

			if (read_len + bytesTransfered >= total_len) {
				//长度够了就调用回调函数
				handler(ec, read_len + bytesTransfered);
				return;
			}

			// 没有错误，且长度不足则继续读取
			self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
		});
}

LogicNode::LogicNode(std::shared_ptr<Session> session, std::shared_ptr<ReceNode> rece_node) : _session(session), _rece_node(rece_node) {}
