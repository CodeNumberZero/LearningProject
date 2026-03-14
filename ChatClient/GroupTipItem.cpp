#include "GroupTipItem.h"

GroupTipItem::GroupTipItem(QWidget *parent) : ListItemBase(parent), _tip("")
{
	ui.setupUi(this);
	SetItemType(ListItemType::GROUP_TIP_ITEM);
}

GroupTipItem::~GroupTipItem()
{}

QSize GroupTipItem::sizeHint() const
{
	return QSize(250, 25); // 返回自定义的尺寸
}

void GroupTipItem::SetGroupTip(QString str)
{
	ui.label->setText(str);
}

