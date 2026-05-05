#include "DistributeLock.h"
#include <thread>
#include <iostream>
#include <string>
#include <chrono>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sw/redis++/redis++.h>

// 使用 Boost UUID 生成全局唯一标识符(UUID);这个标识符会被用作锁的持有者标识符,它确保每个客户端在加锁时拥有唯一的标识,从而能够确保锁的唯一性
static std::string generateUUID() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return to_string(uuid);
}

DistributeLock& DistributeLock::GetInstance() {
    static DistributeLock lock;
    return lock;
}

DistributeLock::~DistributeLock() {

}

// 尝试获取锁,成功返回锁的唯一标识符(UUID),如果获取失败则返回空字符串
std::string DistributeLock::acquireLock(std::shared_ptr<sw::redis::Redis>& redis, const std::string& lockName, int lockTimeout, int acquireTimeout) {
    /*
        redis:用于与Redis服务器通信
        lockName:想要加锁的资源名称
        lockTimeout:锁的有效期,单位是秒.设置这个值的目的是防止因程序异常或崩溃而导致的死锁,当锁达到这个超时时间后,Redis会自动删除这个key,从而释放锁
        acquireTimeout:获取锁的最大等待时间,单位也是秒.如果在这个时间内没有成功获取到锁,函数就会停止尝试,并返回空字符串,这样可以避免程序无限等待
    */
    if (!redis) {
        return "";
    }
    
    std::string identifier = generateUUID();
    std::string lockKey = DISTRIBUTE_LOCK_PREFIX + lockName;   
    auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(acquireTimeout); // 设置获取锁的截止时间

    while (std::chrono::steady_clock::now() < endTime) {                                    // 不断尝试获取锁,直到超时或成功获取锁
        try {
            /*
                1、使用Redis++的set命令,设置NX和EX选项,set(key, value, std::chrono::seconds(lockTimeout), sw::redis::UpdateType::NX)
			    2、对应的原生redis命令:SET key value NX EX lockTimeout
                   NX表示"Not exists",意思是"只有当key不存在时才进行设置",这可以保证如果其他客户端已经设置了这个key(即已经有锁了),那么当前客户端就不会覆盖原来的锁
                   EX参数用于指定key的过期时间,lockTimeout表示锁的有效期(lockTimeout),单位为秒,这样即使客户端因某些原因没有正常释放锁,锁也会在指定时间后自动失效
             */
            bool success = redis->set(lockKey, identifier, std::chrono::seconds(lockTimeout), sw::redis::UpdateType::NOT_EXIST);
            if (success) {
                return identifier;
            }
        }
        catch (const sw::redis::Error& e) {
             std::cerr << "Redis error in DistributeLock's acquireLock: " << e.what() << std::endl;
        }

        // 如果获取锁失败,则暂停1毫秒后重试,防止忙等待,提高 CPU 的利用率
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return "";
}

// 释放锁，只有锁的持有者才能释放，返回是否成功
bool DistributeLock::releaseLock(std::shared_ptr<sw::redis::Redis>& redis, const std::string& lockName, const std::string& identifier) {
    if (!redis) {
        return false;
    }
    
    std::string lockKey = DISTRIBUTE_LOCK_PREFIX + lockName;

    // 使用Redis++的eval命令执行Lua脚本
    std::vector<std::string> keys = { lockKey };
    std::vector<std::string> args = { identifier };

    // Lua脚本:判断锁标识是否匹配,匹配则删除锁(redis只保证单个命令的原子性,而释放锁涉及到查询和删除两个命令,因此要使用Lua脚本将两条命令封装成一条命令执行)
    const std::string lua =
        "if redis.call('get', KEYS[1]) == ARGV[1] then "    // 从Redis获取lockKey对应的值,检查获取到的值是否与传入的identifier相同,只有标识符匹配时才能删除锁
        "  return redis.call('del', KEYS[1]) "              // 如果匹配,执行删除操作,释放锁
        "else "
        "  return 0 "                                       // 如果标识符不匹配,返回0,表示没有成功释放锁
        "end";

    try {
        /*
            1、eval<long long>是Redis++提供的用于执行Lua脚本的模板函数, <long long>指定了脚本的返回类型
            2、对应的原生redis命令:EVAL lua 1 keys args
               lua:要执行的Lua脚本
               1:表示有一个key参数
               keys:包含所有key参数的列表,这里是lockKey
			   args:包含所有arg参数的列表,这里是identifier
        */
        long long result = redis->eval<long long>(lua, keys.begin(),keys.end(), args.begin(), args.end());
        // result是脚本返回值,如果返回1表示删除成功
        return result == 1;                         
    } catch (const sw::redis::Error &e) {
        std::cerr << "Redis error in DistributeLock's releaseLock: " << e.what() << std::endl;
        return false;
    }
}