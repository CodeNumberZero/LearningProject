#pragma once

#include <QWidget>
#include "ui_ApplyFriendPage.h"
#include "UserData.h"
#include "ApplyFriendItem.h"

// 用来显示好友申请列表的类
class ApplyFriendPage : public QWidget
{
	Q_OBJECT

public:
	explicit ApplyFriendPage(QWidget *parent = nullptr);
	~ApplyFriendPage();
    void AddNewApply(std::shared_ptr<AddFriendApply> apply);

protected:
    void paintEvent(QPaintEvent* event);

private:
    void loadApplyList();
    Ui::ApplyFriendPageClass ui;
    std::unordered_map<int, ApplyFriendItem*> _unauth_items;               // 存储未认证的item

public slots:
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);

signals:
    void sig_show_search(bool);
};

