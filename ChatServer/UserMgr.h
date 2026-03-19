#pragma once
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class Session;

// 用于管理用户的管理类
class UserMgr : public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr();
	std::shared_ptr<Session> GetSession(int uid);
	void SetUserSession(int uid, std::shared_ptr<Session> session);
	void RemoveUserSession(int uid);
private:
	UserMgr();
	std::mutex _session_mtx;
	std::unordered_map<int, std::shared_ptr<Session>> _uid_to_session;
};


