#pragma once
#include <grpcpp/grpcpp.h>

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

#include <atomic>
#include <cassert>
#include <functional>
#include <hiredis/hiredis.h>
#include <sw/redis++/redis++.h>
#include <iostream>
#include <memory>
#include <map>
#include <queue>
#include <thread>
#include <unordered_map>

#include "Singleton.h"

constexpr auto CODEPREFIX = "code_";				// 编译期常量表达式，值在编译时就确定
constexpr auto USERIP_PREFIX = "uip_";       
constexpr auto USERTOKEN_PREFIX = "utoken_";
constexpr auto IPCOUNT_PREFIX = "ipcount_";
constexpr auto USER_BASE_INFO = "ubaseinfo_";
constexpr auto LOGIN_COUNT = "logincount";
constexpr auto NAME_INFO = "nameinfo_";
constexpr auto LOCK_PREFIX = "lock_";
constexpr auto USER_SESSION_PREFIX = "usession_";
constexpr auto LOCK_COUNT = "lockcount";
constexpr auto DISTRIBUTE_LOCK_PREFIX = "lock:";	// 分布式锁的key前缀

constexpr int LOCK_TIME_OUT = 10;            // 分布式锁的持有时间(单位为秒)
constexpr int ACQUIRE_TIME_OUT = 5;          // 分布式锁的重试时间(单位为秒)

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,                           // Json解析错误
	RPCFailed = 1002,                            // RPC请求错误
	VarifyCodeExpired = 1003,                    // 验证码过期
	VarifyCodeError = 1004,                      // 验证码错误
	UserExist = 1005,                            // 用户已存在
	PasswdErr = 1006,                            // 密码错误
	EmailNotMatch = 1007,                        // 邮箱不匹配
	PasswdUpdateFailed = 1008,                   // 密码更新失败(重置失败)
	PasswdInvalid = 1009,                        // 密码不合法
	TokenInvalid = 1010,                         // Token失效
	UidInvalid = 1011,                           // uid无效
};

// Defer类:Defer机制,用于在作用域结束时执行某个操作
class Defer {
public:
	// 接受一个lambda表达式或函数指针
	Defer(std::function<void()> func) : _func(func) {}

	// 析构函数中执行传入的函数
	~Defer() {
		_func();
	}

private:
	std::function<void()> _func;
};