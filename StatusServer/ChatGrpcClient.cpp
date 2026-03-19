#include "ChatGrpcClient.h"

ChatConnectionPool::ChatConnectionPool(std::size_t pool_size, std::string host, std::string port)
	: _pool_size(pool_size), _host(host), _port(port), _b_stop(false)
{
	for (std::size_t i = 0; i < _pool_size; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials()); // 创建与服务端的通信通道,第一个参数指定服务端地址;第二个参数表明使用不安全的通道凭证(gRPC通道是可复用的,多个存根可共享一个通道,节省连接资源)
		_connections.push(ChatService::NewStub(channel));                                                              // 通过通道创建ChatService的存根对象(将存根与通道绑定)
		//1、注意,push这一步涉及到了移动语义,一方面NewStub返回的是一个unique_ptr,unique_ptr不能被拷贝,只能通过移动语义将其存入队列;另一方面这里产生的是一个临时右值,只能通过移动语义存入队列
		//2、std::queue的push在C++14后提供了移动重载,可以把临时unique_ptr直接“挪”进去,而不用显式std::move
	}
}

ChatConnectionPool::~ChatConnectionPool()
{
	std::lock_guard<std::mutex> lock(_mutex);
	Close();
	while (!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<ChatService::Stub> ChatConnectionPool::GetConnection() {
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this]() {                         // 若谓词(lambda表达式)返回true:不阻塞,直接退出wait,线程持有锁继续执行后续的取连接逻辑
		if (_b_stop) {
			return true;
		}
		return !_connections.empty();                  // 若谓词返回false:线程释放持有的互斥锁，同时进入阻塞等待状态,直到被notify唤醒后,线程会重新竞争获取互斥锁(_mutex),获取成功后再次执行谓词lambda,重新检查谓词,若返回true则退出wait,线程持有锁继续执行后续的取连接逻辑
		});

	if (_b_stop) {
		return nullptr;
	}
	auto connection = std::move(_connections.front()); // front() 返回的是队首元素的引用,std::move 将这个引用转换为右值引用;队首元素现在变为空(持有nullptr)
	_connections.pop();                                // 移动操作只转移了资源所有权,但队首的 unique_ptr 对象仍然存在于队列中(已经是空指针),pop() 负责从队列中移除这个不再需要的元素对象
	return connection;                                 // 返回值优化机制确保调用时正确返回
}

void ChatConnectionPool::ReturnConnection(std::unique_ptr<ChatService::Stub> connection) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (_b_stop) {
		return;
	}
	_connections.push(std::move(connection));
	_cond.notify_one();
}

void ChatConnectionPool::Close() {
	_b_stop = true;
	_cond.notify_all();
}

ChatGrpcClient::ChatGrpcClient() {
	auto& cfg = ConfigMgr::GetInstance();
	auto server_list = cfg["PeerServer"]["Servers"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {       // 读取直到遇到逗号;字符串分割,用于将以逗号分隔的字符串解析为多个子字符串并存储到vector中
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}
		_pools[cfg[word]["Name"]] = std::make_unique<ChatConnectionPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
	}
}

ChatGrpcClient::~ChatGrpcClient() {

}

/*关于gRPC同一方法的不同参数的相关说明:查看StatusGrpcClient.cpp文件第59行*/

AddFriendRsp ChatGrpcClient::NotifyAddFriend(const AddFriendReq& req)
{
	auto to_uid = req.touid();
	std::string  uid_str = std::to_string(to_uid);

	AddFriendRsp rsp;
	return rsp;
}
