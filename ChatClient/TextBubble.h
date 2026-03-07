#pragma once
#include "global.h"
#include "BubbleFrame.h"

// 用于显示文本消息的气泡类
class TextBubble : public BubbleFrame
{
    Q_OBJECT
public:
    TextBubble(ChatRole role, const QString& text, QWidget* parent = nullptr);
protected:
    bool eventFilter(QObject* o, QEvent* e);                  // 返回值含义：true表示事件已处理，不再传递给目标对象；false表示事件继续正常传递
private:
    void adjustTextHeight();
    void setPlainText(const QString& text);
    void initStyleSheet();
private:
    QTextEdit* m_pTextEdit;
};
