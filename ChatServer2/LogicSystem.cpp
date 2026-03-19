#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"

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
	auto token = root["token"].asString();
	std::cout << "user login uid is  " << uid 
			  << " \nuser token  is "<< token 
			  << std::endl;

	Json::Value rtvalue;
	Defer defer([this, &rtvalue, session] {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	// 从redis获取用户token是否正确(StatusServiceImpl中将uid和对应的token存入了redis中而不是内存,所以从redis中查询)
	// redis++中通过key获取value或者弹出一个key失败时,函数返回值是一个OptionalString类型，其内是空值(将其转换为bool类型,值为0，即false),不能直接使用value()方法或*运算符进行操作,所以需要先判断是否为空
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKEN_PREFIX + uid_str;
	auto token_val = RedisClient::GetInstance()->get(token_key);
	if (!token_val) {                                                        // OptionalString重载了bool()运算符,可以直接进行布尔判断
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	std::string token_value = token_val.value();                             // 使用value()方法或*运算符获取对应的值
	if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return;
	}

	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = GetBaseInfo(base_key, uid, user_info);
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

	// 从数据库获取申请列表

	// 获取好友列表

	auto server_name = ConfigMgr::GetInstance().GetValue("SelfServer", "Name"); // 获取自身服务器的名称
	auto name_val = RedisClient::GetInstance()->hget(LOGIN_COUNT, server_name);      // 从redis中获取服务器的连接数
	int count = 0;
	if (name_val) {
		count = std::stoi(name_val.value());                                         // 若从redis中获取到服务器的连接数,直接转为整型使用
	}

	count++;                                                                    // 将登录数量增加
	auto count_str = std::to_string(count);
	RedisClient::GetInstance()->hset(LOGIN_COUNT, server_name, count_str);
	session->SetUserId(uid);                                                    // session绑定用户uid
	std::string ipkey = USERIP_PREFIX + uid_str;                                // 为用户设置登录ip server的名字
	RedisClient::GetInstance()->set(ipkey, server_name);
	UserMgr::GetInstance()->SetUserSession(uid, session);                       // uid和session绑定管理,方便以后踢人操作

	return;
}

// 根据base_key从redis中查询数据,redis中没有则利用uid从mysql中查询,并将查询结果写入redis
bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	// 优先在redis中查询用户信息
	auto val = RedisClient::GetInstance()->get(base_key);
	if (val) {
		std::string info_str = val.value();
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid 
				  << " \nname  is "<< userinfo->name 
				  << " \npwd is " << userinfo->pwd 
				  << " \nemail is " << userinfo->email 
				  << std::endl;
		return true;
	}
	else {
		// redis中没有则查询mysql数据库
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}
		userinfo = user_info;

		// 将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		//RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
		RedisClient::GetInstance()->set(base_key, redis_root.toStyledString());
		return true;
	}
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
