#include "TextBubble.h"
#include <qtextobject.h>

TextBubble::TextBubble(ChatRole role, const QString& text, QWidget* parent) : BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);                                             // 设置只读模式
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);            // 禁用滚动条
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
        1、安装事件过滤器:将一个对象(filterObj)安装为另一个对象的事件监视器; 被安装的对象的所有事件都会被filterObj先看到; filterObj可以在事件到达目标对象之前拦截和处理事件
        2、如果将当前对象安装为某个对象的事件过滤器,当前对象就必须重写eventFilter,因为installEventFilter只是注册了过滤器,但真正的过滤逻辑必须在eventFilter函数中实现;
           如果没有重写eventFilter,过滤器存在但没有实际作用,所有事件都不会被过滤，直接传递给目标对象
    */
    m_pTextEdit->installEventFilter(this);                                      // 将当前对象安装为m_pTextEdit的事件过滤器,让TextBubble对象监视m_pTextEdit的所有事件,当有事件发生时,会先调用TextBubble::eventFilter函数

    QFont font("Microsoft YaHei");
    font.setPointSize(12);
    m_pTextEdit->setFont(font);
    setPlainText(text);                                                         // 设置文本内容
    setWidget(m_pTextEdit);                                                     // 将文本编辑器设置为气泡的内容控件
    initStyleSheet();
}

// 拉伸的时候要调整气泡的高度,座椅重写事件过滤器，用于在 QTextEdit 绘制时动态调整文本气泡的高度
bool TextBubble::eventFilter(QObject* o, QEvent* e) {
    if (m_pTextEdit == o && e->type() == QEvent::Paint)                         // 检查是否是 m_pTextEdit 的绘制事件
    {
        adjustTextHeight();                                                     // 在绘制事件中调整高度
    }
    return BubbleFrame::eventFilter(o, e);
}

// 精确计算文本内容所需的高度，并调整气泡控件的高度以适应文本
void TextBubble::adjustTextHeight()
{
    qreal doc_margin = m_pTextEdit->document()->documentMargin();               // documentMargin()返回文本内容与 QTextEdit 边框之间的距离,获取文档边距;字体到边框的距离默认为4
    QTextDocument* doc = m_pTextEdit->document();                               // 获取文档对象
    qreal text_height = 0;
    // 把每一段的高度相加 = 文本高
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())        // 遍历所有文本段落，累加高度
    {
        QTextLayout* pLayout = it.layout();                                     // 获取文本段的布局对象
        QRectF text_rect = pLayout->boundingRect();                             // 获取这段文本的边界矩形
        text_height += text_rect.height();                                      // 累加高度
    }
    int vMargin = this->layout()->contentsMargins().top();                      // 获取气泡的垂直边距
    // 设置这个气泡需要的高度 = 文本高 + 文本边距 + TextEdit边框到气泡边框的距离
    setFixedHeight(text_height + doc_margin * 2 + vMargin * 2);                 // 计算总高度并设置
}

// 设置文本内容并计算文本的最大宽度，然后调整气泡的最大宽度以适应最长的文本行
void TextBubble::setPlainText(const QString& text)
{
    m_pTextEdit->setPlainText(text);                                            // 设置纯文本
    // m_pTextEdit->setHtml(text);
    // 获取边距信息,找到段落中最大宽度
    qreal doc_margin = m_pTextEdit->document()->documentMargin();               // 文本到编辑器边框的距离
    int margin_left = this->layout()->contentsMargins().left();                 // 气泡左边距
    int margin_right = this->layout()->contentsMargins().right();               // 气泡右边距
    QFontMetricsF fm(m_pTextEdit->font());                                      // 用于精确测量文本尺寸
    QTextDocument* doc = m_pTextEdit->document();
    int max_width = 0;
    // 遍历每一段找到最宽的那一段
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())        // 字体总长
    {
        int txtW = int(fm.horizontalAdvance(it.text()));                        // 测量当前段落的文本宽度
        max_width = max_width < txtW ? txtW : max_width;                        // 找到最长的那段
    }
    // 设置这个气泡的最大宽度 只需要设置一次
    setMaximumWidth(max_width + doc_margin * 2 + (margin_left + margin_right)); // 设置最大宽度,总宽度 = 最长文本宽度 + 左右文档边距 + 左右布局边距
}

void TextBubble::initStyleSheet() {
    m_pTextEdit->setStyleSheet("QTextEdit{background:transparent;border:none}");// 背景透明，无边框
}