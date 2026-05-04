#pragma once
#include "const.h"

/*
注意：
	1、Redis服务端本身是单线程串行执行所有命令的，天然保证单个命令的原子性，无论多少个客户端连接同时操作同一个Key，Redis都会按命令接收顺序依次执行(类似队列，先进先出)，不会出现多个连接操作同一个key导致的底层执行错误
	2、Redis保证了单个命令的原子性，但多个命令的组合原子性需要开发者自己保证(分布式锁)
*/
class RedisClient {
private:
	RedisClient();                                                       // 如果不写构造函数,系统会生成默认构造,而默认构造函数是public的,会导致单例模式被破坏             
	RedisClient(const RedisClient&) = delete;
	RedisClient& operator=(const RedisClient&) = delete;

	/*
		这里最好声明redis的智能指针作为类成员变量。原因：如果声明redis对象作为成员变量，例如{sw::redis::Redis _redis;}这一句,那么RedisClient类的默认构造函数调用时会先调用Redis的
		默认构造函数,因为在C++中声明对象时会先进行初始化阶段，将所有成员变量初始化后，再进行构造函数的执行阶段，即构造函数中的具体逻辑.因此即便RedisClient的默认构造函数中有实例化redis对象的语句,
		编译器也会先尝试使用默认构造函数初始化一个redis对象,再执行默认构造函数内的具体逻辑,但是redis是没有默认构造函数的(必须传入连接参数),所以会编译报错
	*/
	std::shared_ptr<sw::redis::Redis> _redis;

public:
	~RedisClient();                                                      // 析构设为公有,如果设为私有，析构时就无法调用，这种情况就需要使用辅助类作为删除器来进行析构
	static RedisClient& GetClientInstance();
	static std::shared_ptr<sw::redis::Redis> GetInstance();				 // 返回智能指针的拷贝(而非可修改的引用),避免外部重置内部指针导致隐蔽错误
	std::string acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout);		// 获取分布式锁,成功返回锁的唯一标识符，失败返回空字符串
	bool releaseLock(const std::string& lockName, const std::string& identifier);					// 释放分布式锁,成功返回true，失败返回false
}; 

/*-----------------------------------------------------------------------下面都是基于hiredis库封装的对redis的操作函数和连接池(项目中并没有使用)--------------------------------------------------------*/
// Redis连接池类
class RedisConnectionPool {
public:
	RedisConnectionPool(std::size_t poolSize, const char* host, int port, const char* pwd);
	~RedisConnectionPool();
	redisContext* getConnection();
	void returnConnection(redisContext* context);
	void Close();

private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	const char* host_;
	int port_;
	std::queue<redisContext*> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};


// 基于hiredis库封装的redis操作类(实际上安装了redis plus plus就可以直接调用相关的函数，不需要自己封装)
// 注意：
//	1、Redis服务端本身是单线程串行执行所有命令的，天然保证单个命令的原子性，无论多少个客户端连接同时操作同一个Key，Redis都会按命令接收顺序依次执行(类似队列，先进先出)，不会出现多个连接操作同一个key导致的底层执行错误
//  2、Redis保证了单个命令的原子性，但多个命令的组合原子性需要开发者自己保证(分布式锁)
class RedisMgr : public Singleton<RedisMgr>,
	public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);
	void Close();
private:
	RedisMgr();
	std::unique_ptr<RedisConnectionPool>  _con_pool;
};


