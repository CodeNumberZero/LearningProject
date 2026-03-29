#pragma once
#include "const.h"
#include "data.h"
#include <mysqlx/xdevapi.h>

// MySQL连接对象
class SqlConnection {
public:
	SqlConnection(std::unique_ptr<mysqlx::Session> connection, int64_t lasttime) :_con(std::move(connection)), _last_oper_time(lasttime) {}
	std::unique_ptr<mysqlx::Session> _con;
	int64_t _last_oper_time;                                                       // 用于连接池的定时保活检测,判断连接是否长时间未使用
};

// MySQL连接池
class MysqlPool {
private:
	std::string _host;
	unsigned short _port;
	std::string _user;
	std::string _passwd;
	std::string _schema;                                        // 使用哪个数据库
	int _poolSize;
	std::queue<std::unique_ptr<SqlConnection>> _pool;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::atomic<bool> _b_stop;
	std::thread _check_thread;                                  // 检测线程，每隔一段时间检测连接池中的连接是否存活

public:
	MysqlPool(const std::string& host, unsigned short port, const std::string& user, const std::string& passwd, const std::string& schema, int poolSize);
	~MysqlPool();
	void checkConnectionAlive();                                // 检测连接是否存活
	std::unique_ptr<SqlConnection> getConnection();
	void returnConnection(std::unique_ptr<SqlConnection> connection);
	void Close();
};

// 数据操作类
class MysqlDao
{
private:
	std::unique_ptr<MysqlPool> _pool;

public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);   // 注册用户
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& new_pwd);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	bool AddFriendApply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
	
	bool TestProcedure(const std::string& email, int& uid, std::string& name);

	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
};

