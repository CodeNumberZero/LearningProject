#include "ContactUserItem.h"

ContactUserItem::ContactUserItem(QWidget *parent) : ListItemBase(parent)
{
	ui.setupUi(this);
	SetItemType(ListItemType::CONTACT_USER_ITEM);
	ui.red_point->raise();                            // 将红点置于顶层
	ShowRedPoint(false);                               // 默认不显示红点
}

ContactUserItem::~ContactUserItem()
{}

QSize ContactUserItem::sizeHint() const
{
	return QSize(250, 70);                            // 返回自定义的尺寸
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthInfo> auth_info)
{
    _info = std::make_shared<UserInfo>(auth_info);

    QPixmap pixmap(_info->_icon);                     // 加载图片

	// 设置图片自动缩放
    ui.icon_label->setPixmap(pixmap.scaled(ui.icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); 
    ui.icon_label->setScaledContents(true);

    ui.user_name_lb->setText(_info->_name);
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthRsp> auth_rsp) {
    _info = std::make_shared<UserInfo>(auth_rsp);

    // 加载图片
    QPixmap pixmap(_info->_icon);

    // 设置图片自动缩放
    ui.icon_label->setPixmap(pixmap.scaled(ui.icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui.icon_label->setScaledContents(true);

    ui.user_name_lb->setText(_info->_name);
}

void ContactUserItem::SetInfo(int uid, QString name, QString icon)
{
    _info = std::make_shared<UserInfo>(uid, name, name, icon, 0);

    // 加载图片
    QPixmap pixmap(_info->_icon);

    // 设置图片自动缩放
    ui.icon_label->setPixmap(pixmap.scaled(ui.icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui.icon_label->setScaledContents(true);

    ui.user_name_lb->setText(_info->_name);
}

void ContactUserItem::ShowRedPoint(bool show)
{
    if (show) {
        ui.red_point->show();
    }
    else {
        ui.red_point->hide();
    }

}

std::shared_ptr<UserInfo> ContactUserItem::GetInfo()
{
    return _info;
}