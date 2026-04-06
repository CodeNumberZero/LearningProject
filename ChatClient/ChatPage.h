#pragma once

#include <QWidget>
#include "ui_ChatPage.h"
#include "UserData.h"

class ChatPage : public QWidget
{
	Q_OBJECT

public:
	ChatPage(QWidget *parent = nullptr);
	~ChatPage();
	void SetUserInfo(std::shared_ptr<UserInfo> user_info);
	void AppendChatMsg(std::shared_ptr<TextChatData> msg);

private:
	Ui::ChatPageClass ui;
	std::shared_ptr<UserInfo> _user_info;                       // 存储当前聊天对象的信息，主要用于在界面上显示聊天对象的昵称和头像等信息
	QMap<QString, QWidget*>  _bubble_map;

protected:                                                      // protected的好处就是子类可调用父类，又能保护封装性
	void paintEvent(QPaintEvent* event) override;               // 因为ChatPage继承了QWidget,而QWidget是很基本的组件,所以我们想实现更复杂的样式更新，就需要重写paintEvent(主要作用是用于正确绘制自定义 QWidget 的背景样式,确保控件的外观与当前样式一致)

private slots:
	void on_send_Button_clicked();

signals:
	void sig_append_send_chat_msg(std::shared_ptr<TextChatData> msg);
};

