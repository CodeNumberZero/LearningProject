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
