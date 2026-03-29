#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "MysqlDao.h"

RedisClient::RedisClient()
{
	auto& gCfgMgr = ConfigMgr::GetInstance();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];

	sw::redis::ConnectionOptions ConOpts;                                    // redis连接参数
	ConOpts.host = host;
	ConOpts.port = atoi(port.c_str());
	ConOpts.password = pwd;

	sw::redis::ConnectionPoolOptions PoolOpts;                               // 连接池参数
	PoolOpts.size = 5;                                                       // 连接池大小(包括使用中的和空闲的)

	/*
		多个线程共享同一个Redis实例;
		每个线程执行的Redis命令会自动绑定到池分配的空闲连接，执行完成后连接自动归还池，无需手动释放
		连接池、客户端、命令结果的资源均由 Redis++ 自动管理，无需手动关闭连接、释放对象
	*/                                                            
	// 这里不要使用auto,否则_redis会成为构造函数内的局部变量，而不是给成员变量赋值
	_redis = std::make_shared<sw::redis::Redis>(ConOpts, PoolOpts);          // 传入连接参数和池参数创建Redis客户端(Redis++客户端是线程安全的，可多线程同时调用)
}

RedisClient::~RedisClient()
{
}

std::shared_ptr<sw::redis::Redis>& RedisClient::GetInstance() {
	static RedisClient instance;
	return instance._redis;
}


RedisConnectionPool::RedisConnectionPool(size_t poolSize, const char* host, int port, const char* pwd)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
	for (size_t i = 0; i < poolSize_; ++i) {
		auto* context = redisConnect(host, port);
		if (context == nullptr || context->err != 0) {
			if (context != nullptr) {
				redisFree(context);
			}
			continue;
		}

		auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
		if (reply->type == REDIS_REPLY_ERROR) {
			std::cout << "认证失败" << std::endl;
			//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
			freeReplyObject(reply);
			continue;
		}

		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(reply);
		std::cout << "认证成功" << std::endl;
		connections_.push(context);
	}
}

RedisConnectionPool::~RedisConnectionPool()
{
	std::lock_guard<std::mutex> lock(mutex_);
	while (!connections_.empty()) {
		auto* context = connections_.front();
		redisFree(context);
		connections_.pop();
	}
}


redisContext* RedisConnectionPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] {
		if (b_stop_) {
			return true;
		}
		return !connections_.empty();
		});
	//如果停止则直接返回空指针
	if (b_stop_) {
		return  nullptr;
	}
	auto* context = connections_.front();
	connections_.pop();
	return context;
}

void RedisConnectionPool::returnConnection(redisContext* context)
{
	std::lock_guard<std::mutex> lock(mutex_);
	connections_.push(context);
	cond_.notify_one();
}

void RedisConnectionPool::Close()
{
	b_stop_ = true;
	cond_.notify_all();
}

RedisMgr::RedisMgr() {
	auto& gCfgMgr = ConfigMgr::GetInstance();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	_con_pool.reset(new RedisConnectionPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() {
	Close();
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
	if (reply == NULL) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	value = reply->str;
	freeReplyObject(reply);

	std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
	//执行redis命令行
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());

	//如果返回NULL则说明执行失败
	if (NULL == reply)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	//如果执行失败则释放连接
	if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
	freeReplyObject(reply);
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());
	if (reply == nullptr) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	if (reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	value = reply->str;
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}
bool RedisMgr::RPop(const std::string& key, std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
	if (reply == nullptr) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	if (reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	value = reply->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (reply == nullptr) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	if (reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
	if (reply == nullptr) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	if (reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return "";
	}
	auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
	if (reply == nullptr) {
		//freeReplyObject(reply);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		_con_pool->returnConnection(connect);
		return "";
	}
	if (reply->type == REDIS_REPLY_NIL) {
		freeReplyObject(reply);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		_con_pool->returnConnection(connect);
		return "";
	}

	std::string value = reply->str;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

bool RedisMgr::Del(const std::string& key)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
	if (reply == nullptr) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	if (reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		//freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

void RedisMgr::Close()
{
	_con_pool->Close();
}

