#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"

UserMgr:: ~UserMgr() {
	_uid_to_session.clear();
}

std::shared_ptr<Session> UserMgr::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	auto iter = _uid_to_session.find(uid);
	if (iter == _uid_to_session.end()) {
		return nullptr;
	}

	return iter->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	_uid_to_session[uid] = session;
}

void UserMgr::RemoveUserSession(int uid, std::string session_id)
{
	//auto uid_str = std::to_string(uid);
	//// 因为再次登录可能是其他服务器，所以会造成本服务器删除key，其他服务器注册key的情况
	//// 有可能其他服务登录，本服删除key造成找不到key的情况

	////RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	//{
	//	std::lock_guard<std::mutex> lock(_session_mtx);
	//	_uid_to_session.erase(uid);
	//}

	{
		std::lock_guard<std::mutex> lock(_session_mtx);
		auto iter = _uid_to_session.find(uid);
		if (iter == _uid_to_session.end()) {
			return;
		}

		auto session_id_ = iter->second->GetSessionId();// 获取当前uid对应的用户所在的session的id
		// 将本连接对应的session_id_和外部传进来的session_id进行比较是否相等,不相等说明是其他地方登录了
		if (session_id_ != session_id) {				// 外部传进来的seesion_id是通过redis来存的,之所以存在redis中主要是为了保证用户分布式登录时能查到用户具体在哪个服务器的哪个会话中;也就是说这里的比较就是该服务器存储的用户的会话信息与目前实际的用户会话信息是否相同
			return;
		}
		_uid_to_session.erase(uid);
	}

}

UserMgr::UserMgr()
{

}