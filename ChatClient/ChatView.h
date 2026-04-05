#pragma once
#include <qscrollarea.h>
#include <qlayout.h>
#include <qtimer.h>

class ChatView : public QWidget
{
    Q_OBJECT
public:
    ChatView(QWidget* parent = Q_NULLPTR);
    void appendChatItem(QWidget* item);                 // 尾插,添加条目到聊天背景
    void prependChatItem(QWidget* item);                // 头插
    void insertChatItem(QWidget* before, QWidget* item);// 中间插
    void removeAllItem();                               // 清空聊天视图中的所有消息项，但保留最后一个占位控件
protected:
    bool eventFilter(QObject* o, QEvent* e) override;   // 返回值含义：true表示事件已处理，不再传递给目标对象；false表示事件继续正常传递
    void paintEvent(QPaintEvent* event) override;
private slots:
    void onVScrollBarMoved(int min, int max);
private:
    void initStyleSheet();
private:
    //QWidget *m_pCenterWidget;
    QVBoxLayout* m_pVl;
    QScrollArea* m_pScrollArea;
    bool isAppended;
};