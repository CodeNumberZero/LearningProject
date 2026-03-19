#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include "message.grpc.pb.h"
#include "message.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

// 状态服务连接池(和获取验证码的grpc连接池类似):多个线程从这个池子中获取连接与gRPC服务端进行通信,使用完后再放回池子中供其他线程使用
class StatusConnectionPool {
public:
	StatusConnectionPool(std::size_t pool_size, std::string host, std::string port);
	~StatusConnectionPool();
	std::unique_ptr<StatusService::Stub> GetConnection();                               // 获取一个gRPC连接
	void ReturnConnection(std::unique_ptr<StatusService::Stub> context);                // 使用完连接后将连接放回池子
	void Close();

private:
	std::atomic<bool> _b_stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::queue<std::unique_ptr<StatusService::Stub>> _connections;
	std::mutex _mutex;
	std::condition_variable _cond_var;
};

// 跟状态服务器进行通信,用于获取状态服务的grpc客户端
class StatusGrpcClient : public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient();
	GetChatServerRsp GetChatServer(int uid);                                            // 与grpc服务端通信的接口函数
	LoginRsp Login(int uid, std::string token);

private:
	StatusGrpcClient();                                                                 // 如果单例的子类不写构造函数,系统会生成默认构造,而默认构造函数是public的,会导致单例模式被破坏
	std::unique_ptr<StatusConnectionPool> _pool;                                        // gRPC连接池对象指针
};

