#include "BubbleFrame.h"

const int WIDTH_SANJIAO = 8;  // 聊天气泡框旁边的三角宽


BubbleFrame::BubbleFrame(ChatRole role, QWidget* parent) : QFrame(parent), m_role(role), m_margin(3)
{
    // 创建一个布局，根据是自己发送的消息还是别人发送的，做margin分布
    m_pHLayout = new QHBoxLayout();
    if (m_role == ChatRole::Self)
        m_pHLayout->setContentsMargins(m_margin, m_margin, WIDTH_SANJIAO + m_margin, m_margin);
    else
        m_pHLayout->setContentsMargins(WIDTH_SANJIAO + m_margin, m_margin, m_margin, m_margin);

    this->setLayout(m_pHLayout);
}

void BubbleFrame::setMargin(int margin)
{
    Q_UNUSED(margin);
    //m_margin = margin;
}

// 将气泡框内设置文本内容，或者图片内容
void BubbleFrame::setWidget(QWidget* w)
{
    if (m_pHLayout->count() > 0)                                                               // count()返回布局中包含的项目数量
        return;                                                                                // 如果布局中已经有控件,直接返回，不再添加新控件
    else {
        m_pHLayout->addWidget(w);                                                              // 如果布局为空,添加新控件
    }
}

// 绘制气泡
void BubbleFrame::paintEvent(QPaintEvent* e) 
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);                                                                 // 设置NoPen,表示不绘制轮廓线

    if (m_role == ChatRole::Other)
    {
        //画气泡
        QColor bk_color(Qt::white);
        painter.setBrush(QBrush(bk_color));                                                    // 用设置指定颜色的画刷绘制图形
        QRect bk_rect = QRect(WIDTH_SANJIAO, 0, this->width() - WIDTH_SANJIAO, this->height());// 先绘制矩形
        painter.drawRoundedRect(bk_rect, 5, 5);
        //画小三角
        QPointF points[3] = {                                                                  // 再绘制三角形
            QPointF(bk_rect.x(), 12),
            QPointF(bk_rect.x(), 10 + WIDTH_SANJIAO + 2),
            QPointF(bk_rect.x() - WIDTH_SANJIAO, 10 + WIDTH_SANJIAO - WIDTH_SANJIAO / 2),
        };
        painter.drawPolygon(points, 3);
    }
    else
    {
        QColor bk_color(158, 234, 106);
        painter.setBrush(QBrush(bk_color));
        //画气泡
        QRect bk_rect = QRect(0, 0, this->width() - WIDTH_SANJIAO, this->height());
        painter.drawRoundedRect(bk_rect, 5, 5);
        //画三角
        QPointF points[3] = {
            QPointF(bk_rect.x() + bk_rect.width(), 12),
            QPointF(bk_rect.x() + bk_rect.width(), 12 + WIDTH_SANJIAO + 2),
            QPointF(bk_rect.x() + bk_rect.width() + WIDTH_SANJIAO, 10 + WIDTH_SANJIAO - WIDTH_SANJIAO / 2),
        };
        painter.drawPolygon(points, 3);
    }
    return QFrame::paintEvent(e);
}