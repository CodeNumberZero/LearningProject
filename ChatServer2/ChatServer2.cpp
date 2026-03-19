// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "IOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

// 每当服务器chatserver启动后，都要重新设置一下用户连接数管理,并且每个chatserver既要有tcp服务监听也要有grpc服务监听
int main()
{
	// 注意：要想在catch块中也识别server_name,需要在try块外面声明;在try块里面声明只能在try块内可见,catch块中无法识别
	auto& cfg = ConfigMgr::GetInstance();
	auto server_name = cfg["SelfServer"]["Name"];
	try {
		auto pool = IOServicePool::GetInstance();

		// 服务器刚启动,没有任何连接,因此将登陆数量设置为0
		RedisClient::GetInstance()->hset(LOGIN_COUNT, server_name, "0");

		// 定义一个gRPCServer
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;

		// 监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());        // AddListeningPort()方法指定服务器监听的地址和端口
		builder.RegisterService(&service);                                                  // RegisterService()方法注册要实现的具体服务

		// 构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());                      // BuildAndStart()方法创建并启动服务器
		std::cout << "ChatService RPC Server listening on " << server_address << std::endl;

		// 单独启动一个线程处理grpc服务
		std::thread grpc_server_thread([&server]() {
			server->Wait();
			});

		boost::asio::io_context io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});
		auto port_str = cfg["SelfServer"]["Port"];
		Server s(io_context, atoi(port_str.c_str()));
		io_context.run();

		RedisClient::GetInstance()->hdel(LOGIN_COUNT, server_name);          // 删掉(清空)redis中该服务器的连接数
		RedisClient::GetInstance().reset();                                  // 关闭连接池
		grpc_server_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "ChatServer's Exception: " << e.what() << std::endl;
		RedisClient::GetInstance()->hdel(LOGIN_COUNT, server_name);
		RedisClient::GetInstance().reset();
	}
}