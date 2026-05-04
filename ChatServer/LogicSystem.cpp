#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "CServer.h"

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
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);                   // 将消息id与对应的回调函数绑定起来
	_fun_callback[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_fun_callback[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_fun_callback[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_fun_callback[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	std::cout << "user login uid is  " << uid 
			  << " \nuser token  is "<< token << std::endl;

	Json::Value rtvalue;
	Defer defer([this, &rtvalue, session] {                                  // LogicSystem::LoginHandler()函数执行结束后无论是走哪个分支结束的(即无论成功还是失败)都会把数据发送给客户端
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	/*************		一、判断token和uid是否合理		****************************/
	// 从redis获取用户token是否正确(StatusServiceImpl中将uid和对应的token存入了redis中而不是内存,所以从redis中查询)
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKEN_PREFIX + uid_str;
	auto token_val = RedisClient::GetInstance()->get(token_key);			 // redis++中通过key获取value或者弹出一个key失败时,函数返回值是一个OptionalString类型，其内是空值(将其转换为bool类型,值为0，即false),不能直接使用value()方法或*运算符进行操作,所以需要先判断是否为空
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

	/*************		二、加载用户基本信息以及相应的好友列表和申请列表		****************************/
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
	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid, apply_list);
	if (b_apply) {
		for (auto& apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["icon"] = apply->_icon;
			obj["nick"] = apply->_nick;
			obj["sex"] = apply->_sex;
			obj["desc"] = apply->_desc;
			obj["status"] = apply->_status;
			rtvalue["apply_list"].append(obj);
		}
	}

	// 从数据库获取好友列表
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		rtvalue["friend_list"].append(obj);
	}

	/*************		三、根据uid构造分布式锁key,然后实现分布式锁加锁操作		****************************/
	// 此处添加分布式锁,让该线程独占登录
	auto lock_key = LOCK_PREFIX + uid_str;
	auto identifier = RedisClient::GetClientInstance().acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	// todo:这里没有对是否成功获取到锁进行判断

	// 利用defer机制解锁
	Defer defer_lock([this, identifier, lock_key]() {
		RedisClient::GetClientInstance().releaseLock(lock_key, identifier);
	});

	// 此处判断该用户是否在别处或者本服务器登录
	auto uid_ip_key = USERIP_PREFIX + uid_str;
	auto uid_ip_val = RedisClient::GetInstance()->get(uid_ip_key);				// 函数返回值是一个OptionalString类型
	if (uid_ip_val) {															// OptionalString重载了bool()运算符,可以直接进行布尔判断
		auto uid_ip_value = uid_ip_val.value();									// 使用value()方法或*运算符获取对应的值

		auto& cfg = ConfigMgr::GetInstance();									
		auto self_name = cfg["SelfServer"]["Name"];								// 获取当前服务器ip信息
		if (uid_ip_value == self_name) {										// 如果之前登录的服务器和当前相同,则直接在本服务器踢掉,只需要通过线程锁控制好并发逻辑即可
			auto old_session = UserMgr::GetInstance()->GetSession(uid);			// 查找旧有的连接
			if (old_session) {
				old_session->NotifyOffline(uid);
				_p_server->ClearSession(old_session->GetSessionId());
			}
		}
		else {
			// 如果不是本服务器,则通知grpc通知其他服务器踢掉
		}
	}

	/*************		四、更新redis中的信息		****************************
	*登录成功后，要将uid和对应的ip信息写入redis,方便以后跨服查找;同时将uid和session关联,这样可以通过uid快速找到session;另外uid对应的session信息也要写入redis*/
	
	// 在redis中更新各个服务器的登陆数量
	auto server_name = ConfigMgr::GetInstance().GetValue("SelfServer", "Name"); // 获取自身服务器的名称
	auto name_val = RedisClient::GetInstance()->hget(LOGIN_COUNT, server_name); // 从redis中获取服务器的连接数
	int count = 0;
	if (name_val) {                                                             // OptionalString重载了bool()运算符,可以直接进行布尔判断
		count = std::stoi(name_val.value());                                    // 若从redis中获取到服务器的连接数,直接转为整型使用
	}
	count++;                                                                    // 将登录数量增加
	auto count_str = std::to_string(count);
	RedisClient::GetInstance()->hset(LOGIN_COUNT, server_name, count_str);		// 将更新后的数量写入redis

	session->SetUserId(uid);                                                    // session绑定用户uid
	std::string ipkey = USERIP_PREFIX + uid_str;                                // 为用户设置登录ip server的名字
	RedisClient::GetInstance()->set(ipkey, server_name);

	UserMgr::GetInstance()->SetUserSession(uid, session);                       // uid和session绑定到本服务中,方便以后踢人操作

	std::string uid_session_key = USER_SESSION_PREFIX + uid_str;
	RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());		// 将uid对应的session信息写入redis,这里对应的就是USerMgr.cpp文件中第48行的注释以及CSession.cpp文件中251行

	return;
}

void LogicSystem::SearchInfo(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid_str = root["uid"].asString();
	std::cout << "User SearchInfo uid is  " << uid_str << std::endl;

	Json::Value  rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	bool b_digit = isPureDigit(uid_str);                                        // 如果是纯数字就是uid，如果不是就是name
	if (b_digit) {                                                              // 因为客户端是从search_lineedit中取的数据,不确定用户输入的是uid还是name,只是传送的时候变量名用的uid而已(见ChatClient项目SearchList.cpp文件slot_item_clicked函数的161行),因此这里要根据输入传过来的是uid还是name进行不同的处理
		GetUserByUid(uid_str, rtvalue);
	}
	else {
		GetUserByName(uid_str, rtvalue);
	}
	return;
}

void LogicSystem::AddFriendApply(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
	std::cout << "user login uid is  " << uid 
			  << " applyname  is " << applyname 
			  << " bakname is " << bakname 
			  << " touid is " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_ADD_FRIEND_RSP);
	});

	// 先更新数据库
	MysqlMgr::GetInstance()->AddFriendApply(uid, touid);

	// 查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIP_PREFIX + to_str;
	auto to_ip_val = RedisClient::GetInstance()->get(to_ip_key);              // 先根据touid去redis中查询对方所在服务器(用于判断对方服务器与自身服务器是否是同一服务器)
	if (!to_ip_val) {                                                          // 如果没有则直接返回(OptionalString重载了bool()运算符,可以直接进行布尔判断)
		return; 
	}
	std::string to_ip_value = to_ip_val.value();                              // 对方所处服务器的ip(或名字)
	auto& cfg = ConfigMgr::GetInstance();
	auto self_name = cfg["SelfServer"]["Name"];

	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	auto apply_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, uid, apply_info);

	// 直接通知对方有申请消息
	if (to_ip_value == self_name) {                                           // 如果对方和自己处于同一服务器
		auto session = UserMgr::GetInstance()->GetSession(touid);             // 直接在本服务器上根据对方uid查找对应session
		if (session) {
			// 如果在内存中则直接利用对方连接的session发送通知给对方
			Json::Value notify;
			notify["error"] = ErrorCodes::Success;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			notify["desc"] = "";
			if (b_info) {
				notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
			}
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}
		return;
	}

	// 如果对方和自己不处于同一服务器,调用gRPC服务与对方所在服务器进行通信
	AddFriendReq add_req;
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	add_req.set_desc("");
	if (b_info) {
		add_req.set_icon(apply_info->icon);
		add_req.set_sex(apply_info->sex);
		add_req.set_nick(apply_info->nick);
	}

	// 发送通知
	ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

// 对方向我申请,我进行认证后服务器会收到该请求
void LogicSystem::AuthFriendApply(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	std::cout << "from " << uid << " auth friend to " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();

	std::string base_key = USER_BASE_INFO + std::to_string(touid);
	bool b_info = GetBaseInfo(base_key, touid, user_info);              // 获取对方的基本信息
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
		rtvalue["uid"] = touid;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);                 // 将申请方的信息返回给认证方
	});

	// 先更新数据库
	MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);

	// 更新数据库添加好友
	MysqlMgr::GetInstance()->AddFriend(uid, touid, back_name);

	// 查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIP_PREFIX + to_str;
	auto to_ip_val = RedisClient::GetInstance()->get(to_ip_key);      // 先根据touid去redis中查询对方所在服务器(用于判断对方服务器与自身服务器是否是同一服务器)
	if (!to_ip_val) {                                                      // 如果没有则直接返回(OptionalString重载了bool()运算符,可以直接进行布尔判断)
		return;
	}
	std::string to_ip_value = to_ip_val.value();                       // 对方所处服务器的ip(或名字)
	auto& cfg = ConfigMgr::GetInstance();
	auto self_name = cfg["SelfServer"]["Name"];
	// 直接通知对方有认证通过消息
	if (to_ip_value == self_name) {                                     // 如果对方和自己处于同一服务器
		auto session = UserMgr::GetInstance()->GetSession(touid);       // 直接在本服务器上根据对方uid查找对应session
		if (session) {
			// 如果在内存中则直接利用对方连接的session发送通知给对方
			Json::Value notify;
			notify["error"] = ErrorCodes::Success;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USER_BASE_INFO + std::to_string(uid);
			auto user_info = std::make_shared<UserInfo>();
			bool b_info = GetBaseInfo(base_key, uid, user_info);
			if (b_info) {
				notify["name"] = user_info->name;
				notify["nick"] = user_info->nick;
				notify["icon"] = user_info->icon;
				notify["sex"] = user_info->sex;
			}
			else {
				notify["error"] = ErrorCodes::UidInvalid;
			}
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}
		return;
	}

	// 如果对方和自己不处于同一服务器,调用gRPC服务与对方所在服务器进行通信
	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);

	// 发送通知
	ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();

	const Json::Value arrays = root["text_array"];

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
	});

	// 查询redis查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIP_PREFIX + to_str;
	auto to_ip_val = RedisClient::GetInstance()->get(to_ip_key);
	if (!to_ip_val) {
		return;
	}
	std::string to_ip_value = to_ip_val.value();

	auto& cfg = ConfigMgr::GetInstance();
	auto self_name = cfg["SelfServer"]["Name"];
	// 直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			// 在内存中则直接发送通知对方
			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}
		return;
	}

	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		std::cout << "content is " << content << std::endl;
		std::cout << "msgid is " << msgid << std::endl;
		auto* text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}

	// 发送通知 todo...
	ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
}

bool LogicSystem::isPureDigit(const std::string& str)
{
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;
	std::string base_key = USER_BASE_INFO + uid_str;

	// 优先从redis中查询用户信息
	std::string info_str = "";
	// redis++中通过key获取value或者弹出一个key失败时,函数返回值是一个OptionalString类型，其内是空值(将其转换为bool类型,值为0，即false),不能直接使用value()方法或*运算符进行操作,所以需要先判断是否为空
	auto info_val = RedisClient::GetInstance()->get(base_key);
	if (info_val) {                                                               // OptionalString重载了bool()运算符,可以直接进行布尔判断
		info_str = info_val.value();                                              // 使用value()方法或*运算符获取对应的值

		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid 
				  << " \nname  is " << name 
				  << " \npwd is " << pwd 
				  << " \nemail is " << email 
			      << " \nicon is " << icon << std::endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
	}

	auto uid = std::stoi(uid_str);
	// redis中没有则查询mysql
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(uid);                          // GetUser函数会将查询到的用户信息存入userInfo中
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	// 将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	redis_root["icon"] = user_info->icon;
	RedisClient::GetInstance()->set(base_key, redis_root.toStyledString());

	// 返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;
	std::string base_key = NAME_INFO + name;

	//优先查redis中查询用户信息
	std::string info_str = "";
	// redis++中通过key获取value或者弹出一个key失败时,函数返回值是一个OptionalString类型，其内是空值(将其转换为bool类型,值为0，即false),不能直接使用value()方法或*运算符进行操作,所以需要先判断是否为空
	auto info_val = RedisClient::GetInstance()->get(base_key);
	if (info_val) {                                                               // OptionalString重载了bool()运算符,可以直接进行布尔判断
		info_str = info_val.value();                                              // 使用value()方法或*运算符获取对应的值

		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		std::cout << "user  uid is  " << uid 
				  << " \nname  is " << name 
				  << " \npwd is " << pwd 
				  << " \nemail is " << email << std::endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}

	// redis中没有则查询mysql
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(name);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	// 将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	RedisClient::GetInstance()->set(base_key, redis_root.toStyledString());

	// 返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;

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

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list)
{
	// 从mysql获取好友申请列表
	return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
	// 从mysql获取好友列表
	return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
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

void LogicSystem::SetServer(std::shared_ptr<Server> p_server)
{
	_p_server = p_server;
}
