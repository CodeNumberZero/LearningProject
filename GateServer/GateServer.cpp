// GateServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "Server.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"

void TestRedisPlusPlus() {
    //sw::redis::ConnectionOptions ConOpts;                                     // redis连接参数
    //ConOpts.host = "127.0.0.1";
    //ConOpts.port = 6380;
    //ConOpts.password = "123456";

    //sw::redis::ConnectionPoolOptions PoolOpts;                               // 连接池参数
    //PoolOpts.size = 5;                                                       // 连接池大小(包括使用中的和空闲的)

    //sw::redis::Redis redis(ConOpts, PoolOpts);                               // 传入连接参数和池参数创建Redis客户端(Redis++客户端是线程安全的，可多线程同时调用)
    //auto redis = std::make_unique<sw::redis::Redis>(ConOpts, PoolOpts);

    std::string value;
    assert(RedisClient::GetInstance()->set("blogWebsite", "llfc.club"));
    assert(RedisClient::GetInstance()->get("blogWebsite").value() == "llfc.club");                 // redis++中相关的操作接口(例如get,rpop等)返回的是一个Optional<string>类型，需要通过value()方法或者*运算符获取实际的string值
    assert(RedisClient::GetInstance()->hset("blogInfo", "blogWebsite", "llfc.club"));
    assert(RedisClient::GetInstance()->hget("blogInfo", "blogWebsite").value() == "llfc.club");
    assert(RedisClient::GetInstance()->exists("blogInfo") == 1);
    assert(RedisClient::GetInstance()->del("blogInfo") == 1);
    assert(RedisClient::GetInstance()->del("blogInfo") == 0);
    assert(RedisClient::GetInstance()->exists("blogInfo") == 0);

	assert(RedisClient::GetInstance()->lpush("lpushKey1", "lpushValue1") == 1);
	assert(RedisClient::GetInstance()->lpush("lpushKey1", "lpushValue2") == 2);
    assert(RedisClient::GetInstance()->lpush("lpushKey1", "lpushValue3") == 3);
    assert(RedisClient::GetInstance()->rpush("lpushKey1", "lpushValue4") == 4);

	assert(RedisClient::GetInstance()->rpop("lpushKey1").value() == "lpushValue4");
    assert(RedisClient::GetInstance()->rpop("lpushKey1").value() == "lpushValue1");
	assert(RedisClient::GetInstance()->lpop("lpushKey1").value() == "lpushValue3");
    assert(RedisClient::GetInstance()->lpop("lpushkey2").value() == "");                           // 如果要弹出一个不存在的key,函数返回值是一个空字符串(或者将其转换为bool类型，值为0，即false)

}

int main()
{
    //TestRedisPlusPlus();
    auto& CfgMgr = ConfigMgr::GetInstance();
    std::string gate_port_str = CfgMgr["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());
    try {
        //unsigned short port = static_cast<unsigned short>(8080);
        boost::asio::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
            if (ec) {
                return;
            }
            ioc.stop();
        });

        std::make_shared<Server>(ioc, gate_port)->Start();
        std::cout << "Gate Server listen on port " << gate_port << std::endl;
        ioc.run();
    }
    catch (std::exception& e) {
        std::cout << "GateServer mian occurred exception. Exception is " << e.what() << std::endl;
        return 0;
    }
}
