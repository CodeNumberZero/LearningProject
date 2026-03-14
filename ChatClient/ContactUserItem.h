#pragma once

#include <QWidget>
#include "ui_ContactUserItem.h"
#include "ListItemBase.h"
#include "UserData.h"

// 联系人列表的item
class ContactUserItem : public ListItemBase
{
	Q_OBJECT

public:
	ContactUserItem(QWidget *parent = nullptr);
	~ContactUserItem();
	QSize sizeHint() const override;
	void SetInfo(std::shared_ptr<AuthInfo> auth_info);
	void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);
	void SetInfo(int uid, QString name, QString icon);
	void ShowRedPoint(bool show = false);
	std::shared_ptr<UserInfo> GetInfo();

private:
	Ui::ContactUserItemClass ui;
	std::shared_ptr<UserInfo> _info;
};

