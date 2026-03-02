#include "ChatUserWid.h"

ChatUserWid::ChatUserWid(QWidget *parent)
	: ListItemBase(parent)
{
	ui.setupUi(this);
	SetItemType(ListItemType::CHAT_USER_ITEM);
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
// 或
//setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
// 或
	//setWindowFlags(Qt::FramelessWindowHint);
}

ChatUserWid::~ChatUserWid()
{}

QSize ChatUserWid::sizeHint() const
{
	return QSize(250, 70);                                         // 返回自定义的尺寸
}

void ChatUserWid::SetInfo(QString name, QString head, QString msg)
{
	_name = name;
	_head = head;
	_msg = msg;
	// 加载图片
	QPixmap pixmap(_head);
	// 设置图片自动缩放
	ui.icon_label->setPixmap(pixmap.scaled(ui.icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	ui.icon_label->setScaledContents(true);

	ui.user_name_label->setText(_name);
	ui.user_chat_label->setText(_msg);
}
