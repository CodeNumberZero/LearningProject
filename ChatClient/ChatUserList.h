#pragma once
#include "global.h"
#include <qscrollbar.h>

class ChatUserList : public QListWidget
{
    Q_OBJECT
public:
    ChatUserList(QWidget* parent = nullptr);
    ~ChatUserList();
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;   // 返回值含义：true表示事件已处理，不再传递给目标对象；false表示事件继续正常传递
signals:
    void sig_loading_chat_user();
};
