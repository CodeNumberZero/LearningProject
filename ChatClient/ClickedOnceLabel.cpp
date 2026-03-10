#include "ClickedOnceLabel.h"

ClickedOnceLabel::ClickedOnceLabel(QWidget* parent) : QLabel(parent)
{
	setCursor(Qt::PointingHandCursor);
}

void ClickedOnceLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(this->text());
        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理(确保基类的默认行为得到执行)
    QLabel::mousePressEvent(event);
}
