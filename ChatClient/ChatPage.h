#pragma once

#include <QWidget>
#include "ui_ChatPage.h"

class ChatPage : public QWidget
{
	Q_OBJECT

public:
	ChatPage(QWidget *parent = nullptr);
	~ChatPage();

private:
	Ui::ChatPageClass ui;

protected:                                                      // protected的好处就是子类可调用父类，又能保护封装性
	void paintEvent(QPaintEvent* event) override;               // 因为ChatPage继承了QWidget,而QWidget是很基本的组件,所以我们想实现更复杂的样式更新，就需要重写paintEvent(主要作用是用于正确绘制自定义 QWidget 的背景样式,确保控件的外观与当前样式一致)

private slots:
	void on_send_Button_clicked();
};

