#include "ChatUserList.h"

ChatUserList::ChatUserList(QWidget* parent) : QListWidget(parent)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);                            // 隐藏水平滚动条
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);                              // 隐藏垂直滚动条
    
    /*
        1、安装事件过滤器:将一个对象(filterObj)安装为另一个对象的事件监视器; 被安装的对象的所有事件都会被filterObj先看到; filterObj可以在事件到达目标对象之前拦截和处理事件
        2、如果将当前对象安装为某个对象的事件过滤器,当前对象就必须重写eventFilter,因为installEventFilter只是注册了过滤器,但真正的过滤逻辑必须在eventFilter函数中实现;
           如果没有重写eventFilter,过滤器存在但没有实际作用,所有事件都不会被过滤，直接传递给目标对象
    */
    this->viewport()->installEventFilter(this);                                            // 将当前对象安装为是视口的事件过滤器,让ChatUserList对象监视视口的所有事件,当有事件发生时,会先调用ChatUserList::eventFilter函数
}

ChatUserList::~ChatUserList()
{
}

// watched 是被监视的对象，也就是当前事件正在发生的对象，watched 的作用：
// 1、识别事件发生的具体对象
// 2、在同一个事件过滤器中区分不同对象的事件
// 3、决定是否处理该对象的事件

// event 是发生的事件对象，包含了事件的详细信息

// viewport() 是 QAbstractScrollArea（所有滚动区域的基类）的一个成员函数，返回视口控件
// 视口是实际显示内容的区域，鼠标事件通常发生在 viewport 上，而不是整个控件上，因为用户是在内容区域（视口）上移动鼠标

bool ChatUserList::eventFilter(QObject* watched, QEvent* event)
{
    // 检查事件是否是鼠标悬浮进入或离开可视区域,即视口区域(注意这是视口概念，要与窗口区分)
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            // 鼠标悬浮，显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else if (event->type() == QEvent::Leave) {
            // 鼠标离开，隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    // 检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 计算滚动步数
        // 设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
        // 检查是否滚动到底部
        QScrollBar* scrollBar = this->verticalScrollBar();
        int maxScrollValue = scrollBar->maximum();
        int currentValue = scrollBar->value();
        //int pageSize = 10; // 每页加载的联系人数量
        if (maxScrollValue - currentValue <= 0) {
            // 滚动到底部，加载新的联系人
            qDebug() << "ChatUserList load more chat user";
            //发送信号通知聊天界面加载更多聊天内容
            emit sig_loading_chat_user();
        }
        return true; // 停止事件传递
    }
    return QListWidget::eventFilter(watched, event);
}
