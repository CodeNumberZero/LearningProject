#include "MysqlMgr.h"

MysqlMgr::MysqlMgr()
{
}

MysqlMgr::~MysqlMgr()
{
}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _mysql_dao.RegUser(name, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email)
{
	return _mysql_dao.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& new_pwd)
{
	return _mysql_dao.UpdatePwd(name, new_pwd);
}

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
	return _mysql_dao.CheckPwd(email, pwd, userInfo);
}

bool MysqlMgr::AddFriendApply(const int& from, const int& to)
{
	return _mysql_dao.AddFriendApply(from, to);
}

bool MysqlMgr::AddFriend(const int& from, const int& to, std::string back_name)
{
	return _mysql_dao.AddFriend(from, to, back_name);
}

bool MysqlMgr::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
	return _mysql_dao.GetApplyList(touid, applyList, begin, limit);
}

bool MysqlMgr::TestProcedure(const std::string& email, int& uid, std::string& name)
{
	return _mysql_dao.TestProcedure(email, uid, name);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
{
	return _mysql_dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(std::string name)
{
	return _mysql_dao.GetUser(name);
}
