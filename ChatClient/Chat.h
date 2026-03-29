#pragma once

#include <QDialog>
#include "ui_Chat.h"
#include "global.h"
#include "StateWidget.h"
#include "UserData.h"

class Chat : public QDialog
{
	Q_OBJECT

public:
	Chat(QWidget *parent = nullptr);
	~Chat();

private:
	Ui::ChatClass ui;
	ChatUIMode _mode;
	ChatUIMode _state;
	bool _b_loading;
	QList<StateWidget*> _lb_list;                               // QList既有链表的性能，又有随机存取的性能

	void AddChatUserList();
	void ShowSearch(bool b_search = false);                     // 根据参数决定是否显示搜索列表，默认不显示搜索列表
	void AddLBGroup(StateWidget* lb);
	void ClearLabelState(StateWidget* lb);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override; // 返回值含义：true表示事件已处理，不再传递给目标对象；false表示事件继续正常传递
	void handleGlobalMousePress(QMouseEvent* event);

public slots:
	void slot_loading_chat_user();
	void slot_side_chat();
	void slot_side_contact();
	void slot_text_changed(const QString& str);
	void slot_friend_apply(std::shared_ptr<AddFriendApply> apply);
};

