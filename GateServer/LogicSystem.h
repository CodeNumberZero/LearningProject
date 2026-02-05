#pragma once
#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandle;

// 用于处理不同逻辑的逻辑类
class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	bool HandleGet(std::string path, std::shared_ptr<HttpConnection> connection);
	bool HandlePost(std::string path, std::shared_ptr<HttpConnection> connection);
	void RegGet(std::string url, HttpHandle handler);                                // 注册get请求
	void RegPost(std::string url, HttpHandle handler);                               // 注册post请求

private:
	LogicSystem();
	std::map<std::string, HttpHandle> _post_handlers;                                // post请求的回调函数,map的key为路由,value为回调函数
	std::map<std::string, HttpHandle> _get_handlers;                                 // get请求的回调函数
};

