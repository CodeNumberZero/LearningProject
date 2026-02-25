#pragma once
#include "const.h"
#include "message.grpc.pb.h"

//using grpc::Server;
//using grpc::ServerBuilder;
//using grpc::ServerContext;
//using grpc::Status;
//using message::GetChatServerReq;
//using message::GetChatServerRsp;
//using message::LoginReq;
//using message::LoginRsp;
//using message::StatusService;

// 用于存储聊天服务器信息的结构体
struct ChatServer {
	std::string host;
	std::string port;
	std::string name;
	int connection_count;                                         // 记录连接该服务器的客户端数量
};

// 状态服务的实现类，继承自StatusService::Service，重写了GetChatServer和Login两个RPC方法
class StatusServiceImpl final : public message::StatusService::Service
{
public:
	StatusServiceImpl();
	grpc::Status GetChatServer(grpc::ServerContext* context, const message::GetChatServerReq* request, message::GetChatServerRsp* reply) override;
	grpc::Status Login(grpc::ServerContext* context, const message::LoginReq* request, message::LoginRsp* reply) override;
private:
	void insertToken(int uid, std::string token);
	ChatServer getChatServer();                                   // 获取连接数最少的聊天服务器
	std::unordered_map<std::string, ChatServer> _servers;         // 存储聊天服务器信息的哈希表，键为服务器名称，值为ChatServer结构体
	std::mutex _server_mtx;
	std::unordered_map<int, std::string> _tokens;                 // 存储用户ID和对应Token的哈希表，键为用户ID，值为Token字符串
	std::mutex _token_mtx;
};

