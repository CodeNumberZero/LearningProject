#pragma once
#include "global.h"

// 和ListItemBase类似,窗口会复用，所以先抽象出一个中间的基类ChatItemBase，用于控制聊天界面的item
class ChatItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ChatItemBase(ChatRole role, QWidget* parent = nullptr);
    void setUserName(const QString& name);
    void setUserIcon(const QPixmap& icon);
    void setWidget(QWidget* w);

private:
    ChatRole m_role;
    QLabel* m_pNameLabel;
    QLabel* m_pIconLabel;
    QWidget* m_pBubble;
};

