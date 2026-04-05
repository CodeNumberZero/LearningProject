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
	bool AddFriendApply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit = 10); // 从数据库中获取好友申请列表
	bool AuthFriendApply(const int& from, const int& to);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info_list);

	bool TestProcedure(const std::string& email, int& uid, std::string& name);

	std::shared_ptr<UserInfo> GetUser(int uid);                                               // 根据用户id获取用户信息
	std::shared_ptr<UserInfo> GetUser(std::string name);                                      // 根据用户name获取用户信息
};

