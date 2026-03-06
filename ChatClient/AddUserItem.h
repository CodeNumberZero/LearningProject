#pragma once

#include <QWidget>
#include "ui_AddUserItem.h"
#include "ListItemBase.h"

class AddUserItem : public ListItemBase
{
	Q_OBJECT

public:
	explicit AddUserItem(QWidget *parent = nullptr);
	~AddUserItem();
	QSize sizeHint() const override;

private:
	Ui::AddUserItemClass ui;
};

