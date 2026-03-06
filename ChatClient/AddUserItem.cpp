#include "AddUserItem.h"

AddUserItem::AddUserItem(QWidget *parent)
	: ListItemBase(parent)
{
	ui.setupUi(this);
	SetItemType(ListItemType::ADD_USER_TIP_ITEM);
}

AddUserItem::~AddUserItem()
{}

QSize AddUserItem::sizeHint() const {
	return QSize(250, 70);                            // 返回自定义的尺寸
}