#pragma once

#include <QWidget>
#include "ui_GroupTipItem.h"
#include "ListItemBase.h"

// 用于标明好友分组标题的item
class GroupTipItem : public ListItemBase
{
	Q_OBJECT

public:
	explicit GroupTipItem(QWidget *parent = nullptr);
	~GroupTipItem();
	QSize sizeHint() const override;
	void SetGroupTip(QString str);

private:
	Ui::GroupTipItemClass ui;
	QString _tip;
};

