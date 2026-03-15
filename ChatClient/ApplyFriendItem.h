#pragma once

#include <QWidget>
#include "ui_ApplyFriendItem.h"
#include "ListItemBase.h"
#include "UserData.h"

class ApplyFriendItem : public ListItemBase
{
	Q_OBJECT

public:
	explicit ApplyFriendItem(QWidget *parent = nullptr);
	~ApplyFriendItem();
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);
    void ShowAddBtn(bool b_show);
    QSize sizeHint() const override;
    int GetUid();

private:
    Ui::ApplyFriendItemClass ui;
    std::shared_ptr<ApplyInfo> _apply_info;
    bool _added;

signals:
    void sig_friend_auth(std::shared_ptr<ApplyInfo> apply_info);
};

