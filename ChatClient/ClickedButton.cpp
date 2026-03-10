#include "ClickedButton.h"

ClickedButton::ClickedButton(QWidget* parent) : QPushButton(parent)     // 没有调用父类构造函数
{
	setCursor(Qt::PointingHandCursor);                                  // 设置光标为小手
	//setFocusPolicy(Qt::NoFocus);                                        // Qt 中用于设置控件的焦点策略的方法，告诉控件不接受键盘焦点
}

ClickedButton::~ClickedButton()
{
}

void ClickedButton::SetState(QString normal, QString hover, QString press)
{
	_normal = normal;
	_hover = hover;
	_press = press;
	setProperty("state", normal);                                      // setProperty在样式表中的工作原理:1、Qt 样式系统解析样式表 2、当需要绘制控件时，检查该控件的所有属性 3、如果属性匹配选择器条件，应用相应的样式规则，最后应用自定义的repolish更新样式
	repolish(this);
	update();
}

void ClickedButton::enterEvent(QEnterEvent* event)
{
	setProperty("state", _hover);
	repolish(this);
	update();
	QPushButton::enterEvent(event);                                    // 调用基类的enterEvent以保证正常的事件处理(确保基类的默认行为得到执行)
}

void ClickedButton::leaveEvent(QEvent* event)
{
	setProperty("state", _normal);
	repolish(this);
	update();
	QPushButton::leaveEvent(event);
}

void ClickedButton::mousePressEvent(QMouseEvent* e)
{
	setProperty("state", _press);
	repolish(this);
	update();
	QPushButton::mousePressEvent(e);
	// 基类的处理会：
	// 1. 设置按下状态
	// 2. 触发 clicked 信号
	// 3. 处理焦点变化
	// 4. 更新 :pressed 伪状态
}

void ClickedButton::mouseReleaseEvent(QMouseEvent* e)
{
	setProperty("state", _hover);
	repolish(this);
	update();
	QPushButton::mouseReleaseEvent(e);
	// 基类的处理会：
	// 1. 清除按下状态
	// 2. 如果鼠标在按钮范围内，发射 clicked() 信号
	// 3. 更新界面状态
}
