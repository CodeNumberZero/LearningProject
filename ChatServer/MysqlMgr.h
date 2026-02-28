#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "data.h"

//数据库管理类：用来实现服务层对接逻辑层的调用
class MysqlMgr : public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
private:
	MysqlMgr();
	MysqlDao _mysql_dao;

public:
	~MysqlMgr();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);   // 注册用户
	bool CheckEmail(const std::string& name, const std::string& email);                       // 检查邮箱
	bool UpdatePwd(const std::string& name, const std::string& new_pwd);                      // 更新(重置)密码
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);      // 检查密码(登录)
	bool TestProcedure(const std::string& email, int& uid, std::string& name);

	std::shared_ptr<UserInfo> GetUser(int uid);                                               // 根据用户id获取用户信息
};

