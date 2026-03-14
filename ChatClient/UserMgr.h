#pragma once
#include "global.h"
#include "Singleton.h"
#include "UserData.h"

class UserMgr : public QObject, public Singleton<UserMgr>, public std::enable_shared_from_this<UserMgr>
{
	Q_OBJECT
private:
	UserMgr();
	int _uid;
	QString _name;
	QString _token;
	int _chat_loaded;
	int _contact_loaded;
	std::shared_ptr<UserInfo> _user_info;
	std::vector<std::shared_ptr<ApplyInfo>> _apply_list;
	std::vector<std::shared_ptr<FriendInfo>> _friend_list;
	QMap<int, std::shared_ptr<FriendInfo>> _friend_map;

public:
	friend class Singleton<UserMgr>;
	~UserMgr();
	void SetName(QString name);
	void SetUid(int uid);
	void SetToken(QString token);
	void SetUserInfo(std::shared_ptr<UserInfo> user_info);
	int GetUid();
	QString GetName();
	QString GetIcon();
	void AppendApplyList(QJsonArray array);
	void AppendFriendList(QJsonArray array);
	std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
	std::vector<std::shared_ptr<FriendInfo>> GetChatListPerPage();
	std::vector<std::shared_ptr<FriendInfo>> GetContactListPerPage();
	void UpdateChatLoadedCount();
	void UpdateContactLoadedCount();
	bool IsLoadChatFin();
	bool IsLoadContactFin();
	bool CheckFriendById(int uid);
	void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
	void AddFriend(std::shared_ptr<AuthInfo> auth_info);

public slots:
	void SlotAddFriendRsp(std::shared_ptr<AuthRsp> rsp);
	void SlotAddFriendAuth(std::shared_ptr<AuthInfo> auth);
};

