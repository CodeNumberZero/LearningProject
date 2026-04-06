#include "ChatGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"

ChatConnectionPool::ChatConnectionPool(std::size_t pool_size, std::string host, std::string port) 
	: _pool_size(pool_size), _host(host), _port(port), _b_stop(false)
{
	for (std::size_t i = 0; i < _pool_size; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials()); // 创建与服务端的通信通道,第一个参数指定服务端地址;第二个参数表明使用不安全的通道凭证(gRPC通道是可复用的,多个存根可共享一个通道,节省连接资源)
		_connections.push(ChatService::NewStub(channel));                                                              // 通过通道创建ChatService的存根对象(将存根与通道绑定)
		//1、注意,push这一步涉及到了移动语义,一方面NewStub返回的是一个unique_ptr,unique_ptr不能被拷贝,只能通过移动语义将其存入队列;另一方面这里产生的是一个临时右值,只能通过移动语义存入队列
		//2、std::queue的push在C++14后提供了移动重载,可以把临时unique_ptr直接“挪”进去,而不用显式std::move
	}
}

ChatConnectionPool::~ChatConnectionPool()
{
	std::lock_guard<std::mutex> lock(_mutex);
	Close();
	while(!_connections.empty()){
		_connections.pop();
	}
}

std::unique_ptr<ChatService::Stub> ChatConnectionPool::GetConnection() {
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this](){                         // 若谓词(lambda表达式)返回true:不阻塞,直接退出wait,线程持有锁继续执行后续的取连接逻辑
		if (_b_stop) {
			return true;
		}
		return !_connections.empty();                  // 若谓词返回false:线程释放持有的互斥锁，同时进入阻塞等待状态,直到被notify唤醒后,线程会重新竞争获取互斥锁(_mutex),获取成功后再次执行谓词lambda,重新检查谓词,若返回true则退出wait,线程持有锁继续执行后续的取连接逻辑
	});

	if (_b_stop) {
		return nullptr;
	}
	auto connection = std::move(_connections.front()); // front() 返回的是队首元素的引用,std::move 将这个引用转换为右值引用;队首元素现在变为空(持有nullptr)
	_connections.pop();                                // 移动操作只转移了资源所有权,但队首的 unique_ptr 对象仍然存在于队列中(已经是空指针),pop() 负责从队列中移除这个不再需要的元素对象
	return connection;                                 // 返回值优化机制确保调用时正确返回
}

void ChatConnectionPool::ReturnConnection(std::unique_ptr<ChatService::Stub> connection) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (_b_stop) {
		return;
	}
	_connections.push(std::move(connection));
	_cond.notify_one();
}

void ChatConnectionPool::Close() {
	_b_stop = true;
	_cond.notify_all();
}

ChatGrpcClient::ChatGrpcClient() {
	auto& cfg = ConfigMgr::GetInstance();
	auto server_list = cfg["PeerServer"]["Servers"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')){       // 读取直到遇到逗号;字符串分割,用于将以逗号分隔的字符串解析为多个子字符串并存储到vector中
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}
		_pools[cfg[word]["Name"]] = std::make_unique<ChatConnectionPool>(5, cfg[word]["Host"], cfg[word]["Port"]); // 每个服务器都构造一个对应其地址和端口的gRPC聊天服务连接池
	}
}

ChatGrpcClient::~ChatGrpcClient() {

}

/*关于gRPC同一方法的不同参数的相关说明:查看StatusGrpcClient.cpp文件第59行*/

// 添加好友时若对方与自己处于不同服务器,则调用此gRPC方法与对方所处服务器通信
AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp;
	Defer defer([&rsp, &req]() {
		rsp.set_error(ErrorCodes::Success);
		rsp.set_applyuid(req.applyuid());
		rsp.set_touid(req.touid());
	});

	auto find_iter = _pools.find(server_ip);                              // 根据聊天服务器查找并获取其对应的gRPC连接池
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();                                    // 从池子中获取一个连接
	Status status = stub->NotifyAddFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
	});

	if (!status.ok()) {
		std::cout << "ChatGrpcClient gRPC调用'通知添加好友'服务失败：" << std::endl;
		std::cout << "错误码(Code)：" << status.error_code() << std::endl;
		std::cout << "错误信息(Message)：" << status.error_message() << std::endl;

		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req)
{
	AuthFriendRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
	});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
	});

	if (!status.ok()) {
		std::cout << "ChatGrpcClient gRPC调用'通知认知好友'服务失败：" << std::endl;
		std::cout << "错误码(Code)：" << status.error_code() << std::endl;
		std::cout << "错误信息(Message)：" << status.error_message() << std::endl;

		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	// 优先查redis中查询用户信息
	auto info_str_val = RedisClient::GetInstance()->get(base_key);
	if (info_str_val) {
		std::string info_str = info_str_val.value();

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
				  << " name  is " << userinfo->name 
				  << " pwd is " << userinfo->pwd 
				  << " email is " << userinfo->email << std::endl;
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
		RedisClient::GetInstance()->set(base_key, redis_root.toStyledString());
	}
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue)
{
	TextChatMsgRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}
	});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
	});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}
