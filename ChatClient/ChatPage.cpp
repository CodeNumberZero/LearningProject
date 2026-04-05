#include "ChatPage.h"
#include <qstyleoption.h>
#include "ChatItemBase.h"
#include "MessageTextEdit.h"
#include "TextBubble.h"
#include "PictureBubble.h"
#include "UserMgr.h"

ChatPage::ChatPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    //设置按钮样式
    ui.receive_Button->SetState("normal", "hover", "press");
    ui.send_Button->SetState("normal", "hover", "press");
    //设置图标样式
    ui.emote_label ->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui.file_label->SetState("normal", "hover", "press", "normal", "hover", "press");
}

ChatPage::~ChatPage()
{}

// 如果不重写paintEvent可能无法加载样式表,背景可能不显示，或者显示不正确
void ChatPage::paintEvent(QPaintEvent * event)
{
    QStyleOption opt;                                            // 创建样式选项对象,用于存储绘制控件所需的各种信息（状态、位置、大小等）
    opt.initFrom(this);                                          // 从当前控件初始化选项
    QPainter p(this);                                            // 创建画家对象,用于在控件上绘制,this 指定绘制的目标设备是当前控件
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);   // 绘制背景
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info) {
    _user_info = user_info;
    // 设置ui界面
    ui.title_label->setText(_user_info->_name);
    ui.chat_data_list->removeAllItem();
    for (auto& msg : user_info->_chat_msgs) {
        AppendChatMsg(msg);
    }
}

void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg)
{
    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    ChatRole role;
    //todo... 添加聊天显示
    if (msg->_from_uid == self_info->_uid) {
        role = ChatRole::Self;
        ChatItemBase* pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));
        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui.chat_data_list->appendChatItem(pChatItem);
    }
    else {
        role = ChatRole::Other;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        auto friend_info = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        if (friend_info == nullptr) {
            return;
        }
        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));
        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui.chat_data_list->appendChatItem(pChatItem);
    }
}


// 点击发送按钮根据不同的类型创建不同的气泡消息
void ChatPage::on_send_Button_clicked() {
    auto pTextEdit = ui.chat_textEdit;
    ChatRole role = ChatRole::Self;
    QString userName = QStringLiteral("赤石英雄");
    QString userIcon = ":/image/resource/head_19.jpg";

    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    for (int i = 0; i < msgList.size(); ++i)
    {
        QString type = msgList[i].msgFlag;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));
        QWidget* pBubble = nullptr;
        if (type == "text")
        {
            pBubble = new TextBubble(role, msgList[i].content);
        }
        else if (type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        }
        else if (type == "file")
        {

        }
        if (pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui.chat_data_list->appendChatItem(pChatItem);
        }
    }
}
