#include "ApplyFriendItem.h"

ApplyFriendItem::ApplyFriendItem(QWidget *parent) : ListItemBase(parent), _added(false)
{
	ui.setupUi(this);
    SetItemType(ListItemType::APPLY_FRIEND_ITEM);
    ui.addBtn->SetState("normal", "hover", "press");
    ui.addBtn->hide();
    connect(ui.addBtn, &ClickedButton::clicked, [this]() {
        emit this->sig_friend_auth(_apply_info);
    });
}

ApplyFriendItem::~ApplyFriendItem()
{}

void ApplyFriendItem::SetInfo(std::shared_ptr<ApplyInfo> apply_info) {
    _apply_info = apply_info;
    // 加载图片
    QPixmap pixmap(_apply_info->_icon);

    // 设置图片自动缩放
    ui.icon_lb->setPixmap(pixmap.scaled(ui.icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui.icon_lb->setScaledContents(true);

    ui.user_name_lb->setText(_apply_info->_name);
    ui.user_chat_lb->setText(_apply_info->_desc);
}

void ApplyFriendItem::ShowAddBtn(bool b_show)
{
    if (b_show) {
        ui.addBtn->show();
        ui.already_add_lb->hide();
        _added = false;
    }
    else {
        ui.addBtn->hide();
        ui.already_add_lb->show();
        _added = true;
    }
}

QSize ApplyFriendItem::sizeHint() const {
	return QSize(250, 80); // 返回自定义的尺寸
}

int ApplyFriendItem::GetUid() {
    return _apply_info->_uid;
}