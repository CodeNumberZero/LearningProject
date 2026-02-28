#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"

LogicSystem::LogicSystem() : _b_stop(false){                      // 构造函数中将停止信息初始化为false，注册消息处理函数并且启动了一个工作线程，工作线程执行DealMsg逻辑。
	RegisterCallBack();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);    // 和bind类似，要将成员函数传给线程需要传递函数地址和对应的对象地址
}

void LogicSystem::DealMsg()
{
	for (;;) {
		std::unique_lock<std::mutex> lock(_logic_mtx);                // 先上锁，确保队列安全(unique_lock能配合condition_variable使用，且更加灵活，能配合条件变量可以随时解锁，而lock_guard不具备解锁功能)

		// 判断队列为空则用条件变量等待(不直接跳过是因为：如果一直循环跳过，对于cpu是很大的开销)
		while (_msg_queue.empty() && !_b_stop) {
			_consume.wait(lock);                                       // 挂起线程同时释放资源并解锁
		}

		// 如果停服，取出逻辑队列所有数据及时处理并退出循环
		if (_b_stop) {
			while (!_msg_queue.empty()) {                              // 如果队列不为空，取出队首节点处理
				auto& msg_node = _msg_queue.front();
				std::cout << "Receive msg id is " << msg_node->_rece_node->GetMsgId() << std::endl;
				auto fun_callback_iter = _fun_callback.find(msg_node->_rece_node->GetMsgId());           // 根据消息节点的消息id查找对应的回调函数
				if (fun_callback_iter == _fun_callback.end()) {                                          // 如果没找到对应的回调函数，则弹出节点，进入下一次循环
					_msg_queue.pop();
					continue;
				}
				fun_callback_iter->second(msg_node->_session, msg_node->_rece_node->GetMsgId(),          // 找到id对应的回调函数，触发消息id对应的回调函数
					std::string(msg_node->_rece_node->_data, msg_node->_rece_node->_total_len));
				_msg_queue.pop();                                                                        // 处理完成，弹出队首节点，进入下一次循环
			}
			break;                                                     // 队列全部处理完，由于已关服，所以从循环中退出(for(;;)这个大循环)
		}

		// 如果没停服且队列中有数据
		auto& msg_node = _msg_queue.front();
		std::cout << "Receive msg id is " << msg_node->_rece_node->GetMsgId() << std::endl;
		auto fun_callback_iter = _fun_callback.find(msg_node->_rece_node->GetMsgId());           // 根据消息节点的消息id查找对应的回调函数
		if (fun_callback_iter == _fun_callback.end()) {                                          // 如果没找到对应的回调函数，则弹出节点，进入下一次循环
			_msg_queue.pop();
			continue;
		}
		fun_callback_iter->second(msg_node->_session, msg_node->_rece_node->GetMsgId(),          // 找到id对应的回调函数，触发消息id对应的回调函数
			std::string(msg_node->_rece_node->_data, msg_node->_rece_node->_total_len));
		_msg_queue.pop();
	}
}

void LogicSystem::RegisterCallBack()
{
	_fun_callback[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);     // 将消息id与对应的回调函数绑定起来
}

void LogicSystem::LoginHandler(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	std::cout << "user login uid is  " << uid << " \nuser token  is "
		<< root["token"].asString() << std::endl;
	// 从状态服务器获取token匹配是否准确
	auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());
	Json::Value rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	rtvalue["error"] = rsp.error();
	if (rsp.error() != ErrorCodes::Success) {
		return;
	}

	auto find_iter = _user.find(uid);
	std::shared_ptr<UserInfo> user_info = nullptr;
	if (find_iter == _user.end()) {
		// 查询数据库
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			rtvalue["error"] = ErrorCodes::UidInvalid;
			return;
		}

		_user[uid] = user_info;
	}
	else {
		user_info = find_iter->second;
	}
	rtvalue["uid"] = uid;
	rtvalue["token"] = rsp.token();
	rtvalue["name"] = user_info->name;
}

LogicSystem::~LogicSystem()
{
	_b_stop = true;
	_consume.notify_one();        // 回收线程需要先唤醒
	_worker_thread.join();        // 阻塞等待线程退出
}

void LogicSystem::PostMsgToQueue(std::shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> lock(_logic_mtx);
	_msg_queue.push(msg);

	// 由于队列为0时我们将线程挂起了，当队列由0到1时需要将线程唤醒
	if (_msg_queue.size() == 1) {
		lock.unlock();
		_consume.notify_one();             // 唤醒线程，线程在处理消息时会上锁(对应DealMsg的逻辑)
	}
}
