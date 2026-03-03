#include "ListItemBase.h"
#include <qstyleoption.h>

ListItemBase::ListItemBase(QWidget* parent) : QWidget(parent)
{
}

void ListItemBase::SetItemType(ListItemType itemType)
{
	_itemType = itemType;
}

ListItemType ListItemBase::GetItemType()
{
	return _itemType;
}

void ListItemBase::paintEvent(QPaintEvent* event)
{
	QStyleOption opt;                                            // 创建样式选项对象,用于存储绘制控件所需的各种信息（状态、位置、大小等）
	opt.initFrom(this);                                          // 从当前控件初始化选项
	QPainter p(this);                                            // 创建画家对象,用于在控件上绘制,this 指定绘制的目标设备是当前控件
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);   // 绘制背景
}
