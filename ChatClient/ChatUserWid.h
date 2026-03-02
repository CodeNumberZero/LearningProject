#pragma once

#include <QWidget>
#include "ui_ChatUserWid.h"
#include "ListItemBase.h"

// 因为很多窗口都会复用，所以先抽象出一个中间的基类ListItemBase，用于控制item
class ChatUserWid : public ListItemBase
{
	Q_OBJECT

public:
	ChatUserWid(QWidget* parent = nullptr);
	~ChatUserWid();
	QSize sizeHint() const override;
	void SetInfo(QString name, QString head, QString msg);

private:
	Ui::ChatUserWidClass ui;
	QString _name;
	QString _head;
	QString _msg;
};

