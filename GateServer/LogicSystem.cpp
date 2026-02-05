#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VarifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"

LogicSystem::~LogicSystem()
{
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> connection)
{
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}
	_get_handlers[path](connection);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> connection)
{
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}
	_post_handlers[path](connection);
	return true;
}

void LogicSystem::RegGet(std::string url, HttpHandle handler)
{
	_get_handlers.insert(std::make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandle handler)
{
	_post_handlers.insert(std::make_pair(url, handler));
}

LogicSystem::LogicSystem()
{
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		boost::beast::ostream(connection->_response.body()) << "Receive get_test request" << std::endl;
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			boost::beast::ostream(connection->_response.body()) << "Param " << i << " key is " << elem.first;
			boost::beast::ostream(connection->_response.body()) << " Param " << i << " value is " << elem.second << std::endl;
		}
	});

	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "Get_varifycode mod recive body is " << body_str << std::endl;
		connection->_response.set(boost::beast::http::field::content_type, "text/json");         // 设置响应头Content-Type:text/json，告诉客户端响应体是符合JSON语法的文本数据,便于客户端解析(需要注意的是,无论设置为text/json还是text/plain,响应体的字节数据本身不变,区别仅在于客户端"如何理解和解析"这些数据)
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);                                   // 解析body_str并存入src_root中
		if (!parse_success || !src_root.isMember("email")) {                                     // 如果解析失败,或src_root中没有key的值为"email"
			std::cout << "Get_varifycode mod failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		message::GetVarifyRsp rsp = VarifyGrpcClient::GetInstance()->GetVarifyCode(email);
		std::cout << "email is " << email << std::endl;
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		boost::beast::ostream(connection->_response.body()) << jsonstr;
		return true;                                                                             // C++允许"返回值可忽略的可调用对象"适配"返回void的函数类型".在本例中std::function要求的调用签名是「返回void」,那么它可以接受任何返回类型的可调用对象(如返回bool/int/std::string的Lambda/函数),因为C++会自动忽略可调用对象的返回值,仅执行其逻辑.所以返回bool值不会出错.
	});

	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "User_register mod recive body is " << body_str << std::endl;
		connection->_response.set(boost::beast::http::field::content_type, "text/json");         // 设置响应头Content-Type:text/json，告诉客户端响应体是符合JSON语法的文本数据,便于客户端解析(需要注意的是,无论设置为text/json还是text/plain,响应体的字节数据本身不变,区别仅在于客户端"如何理解和解析"这些数据)
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);                                   // 解析body_str并存入src_root中
		if (!parse_success) {                                                                    // 如果解析失败
			std::cout << "User_register mod failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 先查找redis中email对应的验证码是否合理
		std::string varify_code = RedisClient::GetInstance()->get(CODEPREFIX + src_root["email"].asString()).value();  // 这里要加个CODEPREFIX的前缀，因为在gRPC服务端设置key和value时key的值为为前缀+邮箱地址 
		if (varify_code == "") {
			std::cout << "Varify code expired or not existed!" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeExpired;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << "Varify code error!" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeError;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 查找数据库判断用户是否存在
		int uuid = MysqlMgr::GetInstance()->RegUser(src_root["user"].asString(), src_root["email"].asString(), src_root["passwd"].asString());
		if (uuid == -1 || uuid == 0) {
			std::cout << "User or email exist!" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = ErrorCodes::Success;
		root["email"] = src_root["email"];
		root["uuid"] = uuid;
		root["user"] = src_root["user"].asString();
		root["passwd"] = src_root["passwd"].asString();
		root["confirm"] = src_root["confirm"].asString();
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		boost::beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	// 重置回调逻辑
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "Reset_pwd mod receive body is " << body_str << std::endl;
		connection->_response.set(boost::beast::http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Reset_pwd mod failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		// 先查找redis中email对应的验证码是否合理
		std::string varify_code = RedisClient::GetInstance()->get(CODEPREFIX + src_root["email"].asString()).value();  // 这里要加个CODEPREFIX的前缀，因为在gRPC服务端设置key和value时key的值为为前缀+邮箱地址 
		// redis++中通过key获取value或者弹出一个key失败时,函数返回值是一个空字符串(或者将其转换为bool类型，值为0，即false)
		if (varify_code == "") {
			std::cout << "Varify code expired or not existed!" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeExpired;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << "Varify code error!" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeError;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//更新密码为最新密码
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpdataFailed;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to update password: " << pwd << std::endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		boost::beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});
}