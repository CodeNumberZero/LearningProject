#pragma once
#include "const.h"
#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::VarifyService;
using message::GetVarifyReq;
using message::GetVarifyRsp;

// gRPC连接池类:多个线程从这个池子中获取连接与gRPC服务端进行通信,使用完后再放回池子中供其他线程使用
class RPCConnectionPool {
public:
	RPCConnectionPool(std::size_t poolSize, std::string host, std::string port);
	~RPCConnectionPool();
	void Close();
	std::unique_ptr<VarifyService::Stub> GetConnection();                         // 获取一个gRPC连接
	void ReturnConnection(std::unique_ptr<VarifyService::Stub> con);              // 使用完连接后将连接放回池子

private:
	std::atomic<bool> _b_stop;                                                    // 连接池是否关闭的标志
	std::size_t _poolSize;
	std::string _host;
	std::string _port;
	std::queue<std::unique_ptr<VarifyService::Stub>> _connection;
	std::condition_variable _cond;
	std::mutex _mutex;
};

// 用于获取验证码的grpc客户端
class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
	friend class Singleton<VarifyGrpcClient>;
public:
	GetVarifyRsp GetVarifyCode(std::string email);                                 // 与grpc服务端通信的接口函数

private: 
	VarifyGrpcClient();                                                            // 如果单例的子类不写构造函数,系统会生成默认构造,而默认构造函数是public的,会导致单例模式被破坏
	//std::unique_ptr<VarifyService::Stub> _stub;									   // gRPC的"存根对象"(用于通信的"信使"),是gRPC客户端的核心,封装了与服务端的通信逻辑
	std::unique_ptr<RPCConnectionPool> _pool;                                      // gRPC连接池对象指针
};



