#include "StatusGrpcClient.h"

StatusConnectionPool::StatusConnectionPool(std::size_t pool_size, std::string host, std::string port)
	: _pool_size(pool_size), _host(host), _port(port), _b_stop(false)
{
	for (std::size_t i = 0; i < _pool_size; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials()); // 创建与服务端的通信通道,第一个参数指定服务端地址;第二个参数表明使用不安全的通道凭证(gRPC通道是可复用的,多个存根可共享一个通道,节省连接资源)
		_connections.push(StatusService::NewStub(channel));                                                            // 通过通道创建StatusService的存根对象(将存根与通道绑定)
		//1、注意,push这一步涉及到了移动语义,一方面NewStub返回的是一个unique_ptr,unique_ptr不能被拷贝,只能通过移动语义将其存入队列;另一方面这里产生的是一个临时右值,只能通过移动语义存入队列
		//2、std::queue的push在C++14后提供了移动重载,可以把临时unique_ptr直接“挪”进去,而不用显式std::move
	}
}

StatusConnectionPool::~StatusConnectionPool()
{
	std::lock_guard<std::mutex> lock(_mutex);
	Close();
	while (!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<StatusService::Stub> StatusConnectionPool::GetConnection() {
	std::unique_lock<std::mutex> lock(_mutex);
	_cond_var.wait(lock, [this]() {                                                          // 若谓词(lambda表达式)返回true:不阻塞,直接退出wait,线程持有锁继续执行后续的取连接逻辑
		if (_b_stop) {
			return true;
		}
		return !_connections.empty();                                                        // 若谓词返回false:线程释放持有的互斥锁，同时进入阻塞等待状态,直到被notify唤醒后,线程会重新竞争获取互斥锁(_mutex),获取成功后再次执行谓词lambda,重新检查谓词,若返回true则退出wait,线程持有锁继续执行后续的取连接逻辑
	});
	// 如果连接池已经关闭,则返回nullptr
	if (_b_stop) {
		return nullptr;
	}
	auto context = std::move(_connections.front());                                          // 队列中存的是unique_ptr,只能通过移动语义获取;front() 返回的是队首元素的引用,std::move 将这个引用转换为右值引用;队首元素现在变为空(持有nullptr)
	_connections.pop();																		 // 移动操作只转移了资源所有权,但队首的 unique_ptr 对象仍然存在于队列中(已经是空指针),pop() 负责从队列中移除这个不再需要的元素对象
	return context;                                                                          // 返回值优化机制确保调用时正确返回
}

void StatusConnectionPool::ReturnConnection(std::unique_ptr<StatusService::Stub> context) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (_b_stop) {
		return;
	}
	_connections.push(std::move(context));
	_cond_var.notify_one();
}

void StatusConnectionPool::Close() {
	_b_stop = true;
	_cond_var.notify_all();
}

StatusGrpcClient::~StatusGrpcClient()
{
}

/*
注意：在编译.proto文件时，对于proto文件中定义的GetChatServer方法，gRPC会生成多套代码。
	因此StatusGrpcClient.cpp中调用GetChatServer方法时，实际上调用的是gRPC生成的客户端存根代码，参数request的类型为引用；
	而StatusServiceImpl.cpp中实现的GetChatServer方法则是gRPC生成的服务器端代码,参数request的类型为指针;
	虽然参数类型不同，但它们都指向同一个GetChatServerReq消息对象，gRPC会在底层处理这些细节，确保客户端和服务器之间的数据正确传输和解析。

	其他的gRPC方法，例如之前的GetVarifyCode、Login方法以及ChatGrpcClient文件中的方法等都是类似的。
*/
GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	ClientContext context;																 // 创建gRPC客户端上下文
	GetChatServerRsp reply;																 // 创建请求对象
	GetChatServerReq request;															 // 创建响应对象
	request.set_uid(uid);

	auto stub = _pool->GetConnection();                                                  // 从连接池中获取一个gRPC连接(存根对象)
	Status status = stub->GetChatServer(&context, request, &reply);                      // 同步调用远程RPC方法(该调用是同步阻塞的,客户端会等待服务端返回结果后才继续执行),返回RPC调用状态
	Defer defer([&stub, this]() {
		_pool->ReturnConnection(std::move(stub));                                        // 调用成功后将连接放回连接池(这里使用std::move是因为：一般情况下实参传递给形参是通过值拷贝,这个过程本质是创建一个实例,初始化其值为实参的值;而unique_ptr不能被拷贝,只能通过std::move将实参的资源所有权转移给形参,实参变为空指针,不再持有资源)
	});
	if (status.ok()) {
		return reply;                                                                    // 调用成功则直接返回服务端响应
	}
	else {
		std::cout << "StatusGrpcClient gRPC调用'获取聊天'服务失败：" << std::endl;
		std::cout << "错误码(Code)：" << status.error_code() << std::endl;
		std::cout << "错误信息(Message)：" << status.error_message() << std::endl;

		reply.set_error(ErrorCodes::RPCFailed);                                          // 调用失败时手动设置响应的错误码再返回
		return reply;
	}
}

LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
	ClientContext context;
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);

	auto stub = _pool->GetConnection();
	Status status = stub->Login(&context, request, &reply);
	Defer defer([&stub, this]() {
		_pool->ReturnConnection(std::move(stub));
	});
	if (status.ok()) {
		return reply;
	}
	else {
		std::cout << "StatusGrpcClient gRPC调用'登录'服务失败：" << std::endl;
		std::cout << "错误码(Code)：" << status.error_code() << std::endl;
		std::cout << "错误信息(Message)：" << status.error_message() << std::endl;

		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::GetInstance();
	std::string host = gCfgMgr["StatusServer"]["Host"];
	std::string port = gCfgMgr["StatusServer"]["Port"];
	_pool.reset(new StatusConnectionPool(5, host, port));
}
