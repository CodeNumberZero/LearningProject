#pragma once

#include <QDialog>
#include "ui_Chat.h"
#include "global.h"

class Chat : public QDialog
{
	Q_OBJECT

public:
	Chat(QWidget *parent = nullptr);
	~Chat();
	void AddChatUserList();

private:
	Ui::ChatClass ui;
	ChatUIMode _mode;
	ChatUIMode _state;
	bool _b_loading;
	void ShowSearch(bool b_search = false);            // 根据参数决定是否显示搜索列表，默认不显示搜索列表
};

