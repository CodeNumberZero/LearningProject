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

private:
	Ui::ChatClass ui;
};

