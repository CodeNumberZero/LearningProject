#pragma once

#include <QDialog>
#include "ui_Chat.h"
#include "global.h"
#include "StateWidget.h"

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
	QList<StateWidget*> _lb_list;                      // QList既有链表的性能，又有随机存取的性能

	void AddChatUserList();
	void ShowSearch(bool b_search = false);            // 根据参数决定是否显示搜索列表，默认不显示搜索列表
	void AddLBGroup(StateWidget* lb);
	void ClearLabelState(StateWidget* lb);

private slots:
	void slot_loading_chat_user();
	void slot_side_chat();
	void slot_side_contact();
	void slot_text_changed(const QString& str);
};

