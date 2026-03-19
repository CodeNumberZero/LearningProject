#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"

std::string generate_unique_string() {
	// 创建UUID对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// 将UUID转换为字符串
	std::string unique_string = to_string(uuid);

	return unique_string;
}

/*
注意：在编译.proto文件时，对于proto文件中定义的GetChatServer方法，gRPC会生成多套代码。
	因此StatusGrpcClient.cpp中调用GetChatServer方法时，实际上调用的是gRPC生成的客户端存根代码，参数request的类型为引用；
	而StatusServiceImpl.cpp中实现的GetChatServer方法则是gRPC生成的服务器端代码,参数request的类型为指针;
	虽然参数类型不同，但它们都指向同一个GetChatServerReq消息对象，gRPC会在底层处理这些细节，确保客户端和服务器之间的数据正确传输和解析。

	其他的gRPC方法，例如之前的GetVarifyCode、Login方法以及ChatGrpcClient和ChatServiceImpl文件中的方法等都是类似的。
*/
grpc::Status StatusServiceImpl::GetChatServer(grpc::ServerContext* context, const message::GetChatServerReq* request, message::GetChatServerRsp* reply)
{
	std::string prefix("fpx status server has received :  ");
	const auto& server = getChatServer();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());                                // 设置好reply的相关参数信息后，gRPC的底层会将reply对象返回给客户端，客户端就可以从reply对象中获取这些信息
	insertToken(request->uid(), reply->token());
	return grpc::Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::GetInstance();
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server;
	}
}

// 负载均衡的简单实现：选择连接数最少的服务器
ChatServer StatusServiceImpl::getChatServer() {
	std::lock_guard<std::mutex> guard(_server_mtx);
	auto minServer = _servers.begin()->second;

	// 从redis中获取服务器的连接数
	auto val = RedisClient::GetInstance()->hget(LOGIN_COUNT, minServer.name);                 // hget就是用来从哈希表中提取特定字段的值;hget返回的是OptionalString，需要检查是否有值
	if (!val) {
		// 不存在则默认设置为最大(这样写会存在问题：只有第一次登录的服务器才会保存登录数量，没登录的那一台默认设为最大值，后续连接就一直登不上，因此需要在登录服务器时就向redis中将服务器的登录数量置为0，见ChatServer.cpp文件27行)
		minServer.connection_count = INT_MAX;                                            
	}
	else {
		std::string count_str = val.value();
		minServer.connection_count = std::stoi(count_str);
	}

	for (auto& server : _servers) {
		if (server.second.name == minServer.name) {
			continue;
		}

		auto val = RedisClient::GetInstance()->hget(LOGIN_COUNT, server.second.name);
		if (!val) {
			server.second.connection_count = INT_MAX;
		}
		else {
			std::string count_str = val.value();
			server.second.connection_count = std::stoi(count_str);
		}

		if (server.second.connection_count < minServer.connection_count) {
			minServer = server.second;
		}
	}

	return minServer;
}

grpc::Status StatusServiceImpl::Login(grpc::ServerContext* context, const message::LoginReq* request, message::LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKEN_PREFIX + uid_str;

	// 从redis中获取uid对应的token
	// redis++中通过key获取value或者弹出一个key失败时,函数返回值是一个OptionalString类型，其内是空值(将其转换为bool类型,值为0，即false),不能直接使用value()方法或*运算符进行操作,所以需要先判断是否为空
	auto val = RedisClient::GetInstance()->get(token_key);
	if (!val) {                                                       // 如果没找到uid对应的token;OptionalString重载了bool()运算符,可以直接进行布尔判断
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}

	std::string token_value = val.value();                            // 使用value()方法或*运算符获取uid对应的token的值
	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	reply->set_error(ErrorCodes::Success);                            // 没有问题则设置响应的相关信息
	reply->set_uid(uid);
	reply->set_token(token);
	return grpc::Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);                
	std::string token_key = USERTOKEN_PREFIX + uid_str;
	RedisClient::GetInstance()->set(token_key, token);                // 将uid和对应的token存入redis，方便后续查询
}

