#pragma once
#include "global.h"
#include "Singleton.h"
#include "UserData.h"

class UserMgr : public QObject, public Singleton<UserMgr>, public std::enable_shared_from_this<UserMgr>
{
	Q_OBJECT
private:
	UserMgr();
	//int _uid;
	//QString _name;
	QString _token;
	int _chat_loaded;                                                             // 已加载到聊天列表的好友数量
	int _contact_loaded;                                                          // 已加载到联系人列表的好友数量
	std::shared_ptr<UserInfo> _user_info;                                         // 存储从服务器接收的用户信息
	std::vector<std::shared_ptr<ApplyInfo>> _apply_list;                          // 存储从服务器接收的好友申请列表
	std::vector<std::shared_ptr<FriendInfo>> _friend_list;                        // 存储从服务器接收的好友列表
	QMap<int, std::shared_ptr<FriendInfo>> _friend_map;                           // key为uid,value为对应的好友用户信息(该数据结构在好友申请和认证时会用到,用来判断某个uid所对应的用户是否在map中,即是否已被添加)

public:
	friend class Singleton<UserMgr>;
	~UserMgr();
	//void SetName(QString name);
	//void SetUid(int uid);
	void SetToken(QString token);
	void SetUserInfo(std::shared_ptr<UserInfo> user_info);
	int GetUid();
	QString GetName();
	QString GetIcon();
	std::shared_ptr<UserInfo> GetUserInfo();
	void AppendApplyList(QJsonArray array);
	void AppendFriendList(QJsonArray array);
	std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
	std::vector<std::shared_ptr<FriendInfo>> GetChatListPerPage();               // 分页获取聊天列表数据，按顺序从好友列表中按页大小截取一部分数据返回(主要用于聊天列表的显示)
	std::vector<std::shared_ptr<FriendInfo>> GetContactListPerPage();            // 分页获取联系人列表数据，按顺序从好友列表中按页大小截取一部分数据返回(主要用于联系人列表的显示)
	void UpdateChatLoadedCount();                                                // 更新已加载到聊天列表的好友数量
	void UpdateContactLoadedCount();                                             // 更新已加载到联系人列表的好友数量
	bool IsLoadChatFin();                                                        // 判断好友列表中的好友是否已全部加载到了聊天列表中
	bool IsLoadContactFin();                                                     // 判断好友列表中的好友是否已全部加载到了联系人列表中
	bool CheckFriendById(int uid);
	void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);                           // 该函数是针对自己的:我同意了对方的好友申请,我将相应信息发给服务器,服务器给我对应的回包,我收到回包后将对方添加到自己的好友列表中,即将对方的相关信息添加到map中
	void AddFriend(std::shared_ptr<AuthInfo> auth_info);                         // 该函数是针对对方的:我同意了对方的好友申请,对方也会收到服务器相应的回包,对方收到回包将我添加到好友列表中(与上一个函数是一个双向的关系)
	bool IsAlreadyApply(int uid);                                                // 根据uid判断是否已经申请过了,避免重复添加同一条好友申请记录
	void AddApplyToList(std::shared_ptr<ApplyInfo> apply);                         // 添加好友申请记录到申请列表中
	std::shared_ptr<FriendInfo> GetFriendById(int uid);
	void AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs);

public slots:
	void SlotAddFriendRsp(std::shared_ptr<AuthRsp> rsp);
	void SlotAddFriendAuth(std::shared_ptr<AuthInfo> auth);
};

