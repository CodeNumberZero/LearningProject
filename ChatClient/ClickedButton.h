#pragma once
#include "global.h"

class ClickedButton : public QPushButton
{
	Q_OBJECT
public:
	ClickedButton(QWidget* parent = nullptr);
	~ClickedButton();
	void SetState(QString normal, QString hover, QString press);
protected:
	virtual void enterEvent(QEnterEvent* event) override;                 // 鼠标进入; override的作用：让编译器帮助检查是否正确地重写了基类函数;可以省略 virtual
	virtual void leaveEvent(QEvent* event) override;                      // 鼠标离开
	void mousePressEvent(QMouseEvent* e) override;                        // 鼠标点击(按下)
	void mouseReleaseEvent(QMouseEvent* e) override;                      // 鼠标释放
private:
	QString _normal;
	QString _hover;
	QString _press;
};

