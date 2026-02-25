#include "VarifyGrpcClient.h"
#include "ConfigMgr.h"

RPCConnectionPool::RPCConnectionPool(std::size_t poolSize, std::string host, std::string port) : _poolSize(poolSize), _host(host), _port(port), _b_stop(false)
{
	for (std::size_t i = 0; i < _poolSize; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());  // 创建与服务端的通信通道,第一个参数指定服务端地址;第二个参数表明使用不安全的通道凭证(gRPC通道是可复用的,多个存根可共享一个通道,节省连接资源)
		_connection.push(VarifyService::NewStub(channel));                               // 通过通道创建VarifyService的存根对象(将存根与通道绑定).
		/*
			1、注意,push这一步涉及到了移动语义,一方面NewStub返回的是一个unique_ptr,unique_ptr不能被拷贝,只能通过移动语义将其存入队列;另一方面这里产生的是一个临时右值,只能通过移动语义存入队列
			2、std::queue的push在C++14后提供了移动重载,可以把临时unique_ptr直接“挪”进去,而不用显式std::move
		*/
	}
}

RPCConnectionPool::~RPCConnectionPool()
{
	std::lock_guard<std::mutex> lock(_mutex);
	Close();
	while (!_connection.empty()) {
		_connection.pop();
	}
}

void RPCConnectionPool::Close() {
	_b_stop = true;
	_cond.notify_all();
}

std::unique_ptr<VarifyService::Stub> RPCConnectionPool::GetConnection()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this]() {                                                          // 若谓词(lambda表达式)返回true:不阻塞,直接退出wait,线程持有锁继续执行后续的取连接逻辑
		if (_b_stop) {
			return true;
		}
		return !_connection.empty();                                                     // 若谓词返回false:线程释放持有的互斥锁，同时进入阻塞等待状态,直到被notify唤醒后,线程会重新竞争获取互斥锁(_mutex),获取成功后再次执行谓词lambda,重新检查谓词,若返回true则退出wait,线程持有锁继续执行后续的取连接逻辑
		});

	if (_b_stop) {
		return nullptr;
	}

	auto con = std::move(_connection.front());                                           // 队列中存的是unique_ptr,只能通过移动语义获取
	_connection.pop();
	return con;
}

void RPCConnectionPool::ReturnConnection(std::unique_ptr<VarifyService::Stub> con)
{
	std::lock_guard<std::mutex> lock(_mutex);
	if (_b_stop) {
		return;
	}

	_connection.push(std::move(con));
	_cond.notify_one();
}

GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email)
{
	ClientContext context;																 // 创建gRPC客户端上下文
	GetVarifyReq request;																 // 创建请求对象
	GetVarifyRsp reply;																	 // 创建响应对象
	request.set_email(email);

	auto stub = _pool->GetConnection();                                                 // 从连接池中获取一个gRPC连接(存根对象)
	Status status = stub->GetVarifyCode(&context, request, &reply);                     // 同步调用远程RPC方法(该调用是同步阻塞的,客户端会等待服务端返回结果后才继续执行),返回RPC调用状态
	if (status.ok()) {
		_pool->ReturnConnection(std::move(stub));                                       // 调用成功后将连接放回连接池(这里使用std::move是因为：一般情况下实参传递给形参是通过值拷贝,这个过程本质是创建一个实例,初始化其值为实参的值;而unique_ptr不能被拷贝,只能通过std::move将实参的资源所有权转移给形参,实参变为空指针,不再持有资源)
		return reply;                                                                    // 调用成功则直接返回服务端响应
	}
	else {
		_pool->ReturnConnection(std::move(stub));                                       // 调用成功后将连接放回连接池
		std::cout << "gRPC 调用获取验证码失败：" << std::endl;
		std::cout << "错误码(Code)：" << status.error_code() << std::endl;
		std::cout << "错误信息(Message)：" << status.error_message() << std::endl;

		reply.set_error(ErrorCodes::RPCFailed);                                          // 调用失败时手动设置响应的错误码再返回
		return reply;
	}
}

VarifyGrpcClient::VarifyGrpcClient()
{                   
	auto& CfgMgr = ConfigMgr::GetInstance();
	std::string grpc_host = CfgMgr["VarifyServer"]["Host"];
	std::string grpc_port = CfgMgr["VarifyServer"]["Port"];
	_pool.reset(new RPCConnectionPool(5, grpc_host, grpc_port));
}

