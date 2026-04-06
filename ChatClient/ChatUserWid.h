#pragma once

#include <QWidget>
#include "ui_ChatUserWid.h"
#include "ListItemBase.h"
#include "UserData.h"

// 因为很多窗口都会复用，所以先抽象出一个中间的基类ListItemBase，用于控制item
class ChatUserWid : public ListItemBase
{
	Q_OBJECT

public:
	ChatUserWid(QWidget* parent = nullptr);
	~ChatUserWid();
	QSize sizeHint() const override;
	void SetInfo(std::shared_ptr<UserInfo> user_info);
	void SetInfo(std::shared_ptr<FriendInfo> friend_info);
	std::shared_ptr<UserInfo> GetUserInfo();
	//void ShowRedPoint(bool bshow);
	void updateLastMsg(std::vector<std::shared_ptr<TextChatData>> msgs);   // 在聊天列表的聊天条目中显示两人聊天的最后一条消息内容

private:
	Ui::ChatUserWidClass ui;
	std::shared_ptr<UserInfo> _user_info;
};

