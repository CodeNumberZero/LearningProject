#include "const.h"
#include "ConfigMgr.h"
//#include "RedisMgr.h"
//#include "MysqlMgr.h"
//#include "IOServicePool.h" 
#include "StatusServiceImpl.h"

void RunServer() {
	auto& cfg = ConfigMgr::GetInstance();

	std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
	StatusServiceImpl service;

	grpc::ServerBuilder builder;                                                                             // grpc::ServerBuilder是gRPC库自带的类
	// 监听端口和添加服务
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());                             // AddListeningPort()方法指定服务器监听的地址和端口
	builder.RegisterService(&service);                                                                       // RegisterService()方法注册要实现的具体服务

	// 构建并启动gRPC服务器
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());                                           // BuildAndStart()方法创建并启动服务器
	std::cout << "Stauts server listening on " << server_address << std::endl;

	// 创建Boost.Asio的io_context
	boost::asio::io_context io_context;
	// 创建signal_set用于捕获SIGINT
	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

	// 设置异步等待SIGINT信号
	signals.async_wait([&server, &io_context](const boost::system::error_code& error, int signal_number) {
		if (!error) {
			std::cout << "Shutting down server..." << std::endl;
			server->Shutdown(); // 优雅地关闭服务器
			io_context.stop();  // 停止io_context
		}
	});

	// 在单独的线程中运行io_context
	std::thread([&io_context]() { io_context.run(); }).detach();

	// 等待服务器关闭
	server->Wait();

}

int main(int argc, char** argv) {
	try {
		RunServer();
	}
	catch (std::exception const& e) {
		std::cerr << "Status server error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}