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

const int MAX_LENGTH = 1024 * 2;             // 消息最大长度
const int HEAD_ID_LENGTH = 2;                // 头部中消息id的长度(因为是short类型所以是2字节)
const int HEAD_DATA_LENGTH = 2;              // 头部中存储消息长度的变量的长度(也是short类型)
const int HEAD_TOTAL_LENGTH = 4;             // 头部的总长度(HEAD_ID_LENGTH + HEAD_DATA_LENGTH)

const int MAX_SEND_QUE_SIZE = 1000;          // 发送队列最大容量
const int MAX_RECE_QUE_SIZE = 10000;         // 接收队列最大容量

constexpr auto USERIP_PREFIX = "uip_";       // 编译期常量表达式，值在编译时就确定
constexpr auto USERTOKEN_PREFIX = "utoken_";
constexpr auto IPCOUNT_PREFIX = "ipcount_";
constexpr auto USER_BASE_INFO = "ubaseinfo_";
constexpr auto LOGIN_COUNT = "logincount";
constexpr auto NAME_INFO = "nameinfo_";
constexpr auto LOCK_PREFIX = "lock_";
constexpr auto USER_SESSION_PREFIX = "usession_";
constexpr auto LOCK_COUNT = "lockcount";

constexpr int LOCK_TIME_OUT = 10;            // 分布式锁的持有时间
constexpr int ACQUIRE_TIME_OUT = 5;          // 分布式锁的重试时间

enum MSG_ID {
	MSG_CHAT_LOGIN = 1005,                   // 用户登陆
	MSG_CHAT_LOGIN_RSP = 1006,				 // 用户登陆回包
	ID_SEARCH_USER_REQ = 1007,				 // 用户搜索请求
	ID_SEARCH_USER_RSP = 1008,				 // 搜索用户回包
	ID_ADD_FRIEND_REQ = 1009,				 // 申请添加好友请求
	ID_ADD_FRIEND_RSP = 1010,				 // 申请添加好友回复
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,		 // 通知用户添加好友申请
	ID_AUTH_FRIEND_REQ = 1013,				 // 认证好友请求
	ID_AUTH_FRIEND_RSP = 1014,				 // 认证好友回复
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015,		 // 通知用户认证好友申请
	ID_TEXT_CHAT_MSG_REQ = 1017,			 // 文本聊天信息请求
	ID_TEXT_CHAT_MSG_RSP = 1018,			 // 文本聊天信息回复
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,		 // 通知用户文本聊天信息
	ID_NOTIFY_OFF_LINE_REQ = 1021,			 // 通知用户下线
	ID_HEART_BEAT_REQ = 1023,				 // 心跳请求
	ID_HEARTBEAT_RSP = 1024,				 // 心跳回复
};

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,                       // Json解析错误
	RPCFailed = 1002,						 // RPC请求错误
	VarifyExpired = 1003,					 // 验证码过期
	VarifyCodeErr = 1004,					 // 验证码错误
	UserExist = 1005,						 // 用户已经存在
	PasswdErr = 1006,						 // 密码错误
	EmailNotMatch = 1007,					 // 邮箱不匹配
	PasswdUpFailed = 1008,					 // 密码更新失败(重置失败)
	PasswdInvalid = 1009,					 // 密码不合法
	TokenInvalid = 1010,					 // Token失效
	UidInvalid = 1011,						 // uid无效
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