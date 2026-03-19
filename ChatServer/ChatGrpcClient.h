#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "data.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

// 聊天服务连接池(和获取验证码的grpc连接池类似):多个线程从这个池子中获取连接与gRPC服务端进行通信,使用完后再放回池子中供其他线程使用
class ChatConnectionPool {
public:
	ChatConnectionPool(std::size_t pool_size, std::string host, std::string port);
	~ChatConnectionPool();
	std::unique_ptr<ChatService::Stub> GetConnection();
	void ReturnConnection(std::unique_ptr<ChatService::Stub> connection);
	void Close();

private:
	std::atomic<bool> _b_stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::queue<std::unique_ptr<ChatService::Stub>> _connections;
	std::mutex _mutex;
	std::condition_variable _cond;
};

// 跟聊天服务器进行通信,用于获取对端聊天服务的grpc客户端
class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient();

	// 与grpc服务端通信的接口函数
	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
private:
	ChatGrpcClient();                                                                 // 如果单例的子类不写构造函数,系统会生成默认构造,而默认构造函数是public的,会导致单例模式被破坏
	//std::unique_ptr<ChatConnectionPool> _pool;                                        // gRPC连接池对象指针
	std::unordered_map<std::string, std::unique_ptr<ChatConnectionPool>> _pools;
};

