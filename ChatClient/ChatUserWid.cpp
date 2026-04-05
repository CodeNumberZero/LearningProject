#include "ChatUserWid.h"

ChatUserWid::ChatUserWid(QWidget *parent)
	: ListItemBase(parent)
{
	ui.setupUi(this);
	SetItemType(ListItemType::CHAT_USER_ITEM);
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
	// 或
	//setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
	// 或
	//setWindowFlags(Qt::FramelessWindowHint);
}

ChatUserWid::~ChatUserWid()
{}

QSize ChatUserWid::sizeHint() const
{
	return QSize(250, 70);                                         // 返回自定义的尺寸
}

void ChatUserWid::SetInfo(std::shared_ptr<UserInfo> user_info) {
	_user_info = user_info;
	// 加载图片
	QPixmap pixmap(_user_info->_icon);

	// 设置图片自动缩放
	ui.icon_label->setPixmap(pixmap.scaled(ui.icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	ui.icon_label->setScaledContents(true);

	ui.user_name_label->setText(_user_info->_name);
	ui.user_chat_label->setText(_user_info->_last_msg);
}

void ChatUserWid::SetInfo(std::shared_ptr<FriendInfo> friend_info) {
	_user_info = std::make_shared<UserInfo>(friend_info);
	// 加载图片
	QPixmap pixmap(_user_info->_icon);

	// 设置图片自动缩放
	ui.icon_label->setPixmap(pixmap.scaled(ui.icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	ui.icon_label->setScaledContents(true);

	ui.user_name_label->setText(_user_info->_name);
	ui.user_chat_label->setText(_user_info->_last_msg);
}


//void ChatUserWid::ShowRedPoint(bool bshow)
//{
//	if (bshow) {
//		ui->red_point->show();
//	}
//	else {
//		ui->red_point->hide();
//	}
//}

std::shared_ptr<UserInfo> ChatUserWid::GetUserInfo()
{
	return _user_info;
}

//void ChatUserWid::updateLastMsg(std::vector<std::shared_ptr<TextChatData>> msgs) {
//
//	QString last_msg = "";
//	for (auto& msg : msgs) {
//		last_msg = msg->_msg_content;
//		_user_info->_chat_msgs.push_back(msg);
//	}
//
//	_user_info->_last_msg = last_msg;
//	ui->user_chat_lb->setText(_user_info->_last_msg);
//}