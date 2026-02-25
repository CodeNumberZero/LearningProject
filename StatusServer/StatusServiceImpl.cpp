#include "StatusServiceImpl.h"
#include "ConfigMgr.h"

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

	其他的gRPC方法，例如之前的GetVarifyCode、Login方法等都是类似的。
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
	ChatServer server;
	server.port = cfg["ChatServer1"]["Port"];
	server.host = cfg["ChatServer1"]["Host"];
	server.connection_count = 0;
	server.name = cfg["ChatServer1"]["Name"];
	_servers[server.name] = server;

	server.port = cfg["ChatServer2"]["Port"];
	server.host = cfg["ChatServer2"]["Host"];
	server.name = cfg["ChatServer2"]["Name"];
	server.connection_count = 0;
	_servers[server.name] = server;
}

// 负载均衡的简单实现：选择连接数最少的服务器
ChatServer StatusServiceImpl::getChatServer() {
	std::lock_guard<std::mutex> guard(_server_mtx);
	auto minServer = _servers.begin()->second;
	// 使用范围基于for循环
	for (const auto& server : _servers) {
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
	std::lock_guard<std::mutex> guard(_token_mtx);
	auto iter = _tokens.find(uid);
	if (iter == _tokens.end()) {                                      // 没找到相应的用户ID，不存在
		reply->set_error(ErrorCodes::UidInvalid);
		return grpc::Status::OK;
	}
	if (iter->second != token) {                                      // 找到了用户ID，但Token不匹配，说明Token无效
		reply->set_error(ErrorCodes::TokenInvalid);
		return grpc::Status::OK;
	}
	reply->set_error(ErrorCodes::Success);                            // 没有问题则设置响应的相关信息
	reply->set_uid(uid);
	reply->set_token(token);
	return grpc::Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::lock_guard<std::mutex> guard(_token_mtx);
	_tokens[uid] = token;
}

