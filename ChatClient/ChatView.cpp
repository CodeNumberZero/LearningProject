#include "ChatView.h"
#include <qdebug.h>
#include <qevent.h>
#include <QPainter>
#include <QStyleOption>
#include <qscrollbar.h>

ChatView::ChatView(QWidget* parent) : QWidget(parent), isAppended(false)
{
    QVBoxLayout* pMainLayout = new QVBoxLayout();
    this->setLayout(pMainLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 0);

    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget* w = new QWidget(this);
    w->setObjectName("chat_background");
    w->setAutoFillBackground(true);

    QVBoxLayout* pVLayout_1 = new QVBoxLayout();
    pVLayout_1->addWidget(new QWidget(), 100000);
    w->setLayout(pVLayout_1);
    m_pScrollArea->setWidget(w);

    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar* pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);

    //把垂直ScrollBar放到上边 而不是原来的并排
    QHBoxLayout* pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setContentsMargins(0, 0, 0, 0);
    m_pScrollArea->setLayout(pHLayout_2);
    pVScrollBar->setHidden(true);

    m_pScrollArea->setWidgetResizable(true);

    /*
        1、安装事件过滤器:将一个对象(filterObj)安装为另一个对象的事件监视器; 被安装的对象的所有事件都会被filterObj先看到; filterObj可以在事件到达目标对象之前拦截和处理事件
        2、如果将当前对象安装为某个对象的事件过滤器,当前对象就必须重写eventFilter,因为installEventFilter只是注册了过滤器,但真正的过滤逻辑必须在eventFilter函数中实现;
           如果没有重写eventFilter,过滤器存在但没有实际作用,所有事件都不会被过滤，直接传递给目标对象
    */
    m_pScrollArea->installEventFilter(this);                                      // 将当前对象安装为m_pScrollArea的事件过滤器,让ChatView对象监视m_pScrollArea的所有事件,当有事件发生时,会先调用ChatView::eventFilter函数
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget* item)
{
    QVBoxLayout* vl = qobject_cast<QVBoxLayout*>(m_pScrollArea->widget()->layout());
    vl->insertWidget(vl->count() - 1, item);
    isAppended = true;
}

void ChatView::prependChatItem(QWidget* item)
{
}

void ChatView::insertChatItem(QWidget* before, QWidget* item)
{
}

// 重写事件过滤器
bool ChatView::eventFilter(QObject* o, QEvent* e)
{
    if (e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    else if (e->type() == QEvent::Leave && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

// 重写paintEvent支持子类绘制
void ChatView::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

// 监听滚动区域变化的槽函数
void ChatView::onVScrollBarMoved(int min, int max) {
    if (isAppended) //添加item可能调用多次
    {
        QScrollBar* pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());
        //500毫秒内可能调用多次
        QTimer::singleShot(500, [this]()
            {
                isAppended = false;
            });
    }
}

void ChatView::initStyleSheet()
{
}
