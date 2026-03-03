#include "ClickedLabel.h"

ClickedLabel::ClickedLabel(QWidget* parent) : QLabel(parent), _curstate(ClickLbState::Normal)
{
}

void ClickedLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (_curstate == ClickLbState::Normal) {
            qDebug() << "clicked , change to selected press: " << _selected_press;
            _curstate = ClickLbState::Selected;
            setProperty("state", _selected_press);                      // setProperty在样式表中的工作原理:1、Qt 样式系统解析样式表 2、当需要绘制控件时，检查该控件的所有属性 3、如果属性匹配选择器条件，应用相应的样式规则，最后应用自定义的repolish更新样式
            repolish(this);  
            update();
        }
        else {
            qDebug() << "clicked , change to normal press: " << _normal_press;
            _curstate = ClickLbState::Normal;
            setProperty("state", _normal_press);
            repolish(this);
            update();
        }
        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理(确保基类的默认行为得到执行)
    QLabel::mousePressEvent(event);
    // 基类的处理会：
    // 1. 设置按下状态
    // 2. 触发 clicked 信号
    // 3. 处理焦点变化
    // 4. 更新 :pressed 伪状态
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (_curstate == ClickLbState::Normal) {
            qDebug()<<"ReleaseEvent , change to normal hover: "<< _normal_hover;
            setProperty("state", _normal_hover);
            repolish(this);
            update();
        }
        else {
            qDebug()<<"ReleaseEvent , change to selected hover: "<< _selected_hover;
            setProperty("state", _selected_hover);
            repolish(this);
            update();
        }
        emit clicked();
        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QLabel::mousePressEvent(event);
}

void ClickedLabel::enterEvent(QEnterEvent* event)
{
    // 在这里处理鼠标悬停进入的逻辑
    if (_curstate == ClickLbState::Normal) {
        qDebug() << "enter , change to normal hover: " << _normal_hover;
        setProperty("state", _normal_hover);
        repolish(this);
        update();
    }
    else {
        qDebug() << "enter , change to selected hover: " << _selected_hover;
        setProperty("state", _selected_hover);
        repolish(this);
        update();
    }
    QLabel::enterEvent(event);
}

void ClickedLabel::leaveEvent(QEvent* event)
{
    // 在这里处理鼠标悬停离开的逻辑
    if (_curstate == ClickLbState::Normal) {
        qDebug() << "leave , change to normal : " << _normal;
        setProperty("state", _normal);
        repolish(this);
        update();
    }
    else {
        qDebug() << "leave , change to normal hover: " << _selected;
        setProperty("state", _selected);
        repolish(this);
        update();
    }
    QLabel::leaveEvent(event);
}

void ClickedLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;
    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;
    setProperty("state", normal);
    repolish(this);
}

ClickLbState ClickedLabel::GetCurState() {
    return _curstate;
}
