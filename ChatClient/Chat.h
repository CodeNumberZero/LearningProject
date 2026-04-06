#pragma once

#include <QDialog>
#include "ui_Chat.h"
#include "global.h"
#include "StateWidget.h"
#include "UserData.h"

class Chat : public QDialog
{
	Q_OBJECT

public:
	Chat(QWidget *parent = nullptr);
	~Chat();

private:
	Ui::ChatClass ui;
	ChatUIMode _mode;
	ChatUIMode _state;
	bool _b_loading;
	QWidget* _last_widget;
	QList<StateWidget*> _lb_list;                               // QList既有链表的性能，又有随机存取的性能
	QMap<int, QListWidgetItem*> _chat_items_added;              // 记录已经添加到聊天列表的聊天条目，key为好友uid，value为对应的聊天条目指针
	int _cur_chat_uid;                                          // 当前选中的聊天条目的好友uid(即当前聊天界面在和谁聊天)

	void AddChatUserList();
	void ShowSearch(bool b_search = false);                     // 根据参数决定是否显示搜索列表，默认不显示搜索列表
	void AddLBGroup(StateWidget* lb);
	void ClearLabelState(StateWidget* lb);
	void SetSelectChatItem(int uid = 0);                        // 根据用户ID选中聊天列表中对应的聊天项
	void SetSelectChatPage(int uid = 0);                        // 根据用户ID选中聊天页面，并在右侧聊天区域显示对应的用户信息
	void loadMoreChatUser();
	void loadMoreContactUser();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override; // 返回值含义：true表示事件已处理，不再传递给目标对象；false表示事件继续正常传递
	void handleGlobalMousePress(QMouseEvent* event);
	void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);

public slots:
	void slot_loading_chat_user();
	void slot_loading_contact_user();
	void slot_side_chat();
	void slot_side_contact();
	void slot_text_changed(const QString& str);
	void slot_friend_apply(std::shared_ptr<AddFriendApply> apply);
	void slot_add_friend_auth(std::shared_ptr<AuthInfo> auth_info);
	void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
	void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
	void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
	void slot_switch_apply_friend_page();
	void slot_jump_chat_item_from_friendinfopage(std::shared_ptr<UserInfo> user_info);
	void slot_item_clicked(QListWidgetItem* item);
	void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
	void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
};

