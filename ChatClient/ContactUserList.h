#pragma once
#include "global.h"
#include "UserData.h"

class ContactUserItem;

class ContactUserList : public QListWidget
{
    Q_OBJECT
public:
    ContactUserList(QWidget* parent = nullptr);
    void ShowRedPoint(bool b_show = true);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void addContactUserList();

public slots:
    void slot_item_clicked(QListWidgetItem* item);
    void slot_add_firend_auth(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auto_rsp);

signals:
    void sig_loading_contact_user();
    void sig_switch_apply_friend_page();
    void sig_switch_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void sig_switch_friend_info_page();

private:
    bool _load_pending;
    ContactUserItem* _add_friend_item;
    QListWidgetItem* _groupitem;
};

