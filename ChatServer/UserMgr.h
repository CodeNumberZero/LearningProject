#pragma once
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class Session;

// 用于管理用户的管理类(类中对Session的操作没有加分布式锁,只加了线程锁,因为整体思路是在最外层加分布式锁,而接口内部只加线程锁,保证同一个服务器操作的原子性)
class UserMgr : public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr();
	std::shared_ptr<Session> GetSession(int uid);						// 根据用户uid查询对应的session时仅限于UserMgr所在的服务器上查询
	void SetUserSession(int uid, std::shared_ptr<Session> session);		// 将uid和session绑定
	void RemoveUserSession(int uid, std::string session_id);			// 根据uid和session_id删除对应的session
private:
	UserMgr();
	std::mutex _session_mtx;
	std::unordered_map<int, std::shared_ptr<Session>> _uid_to_session;
};


