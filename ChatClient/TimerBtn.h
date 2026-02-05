#pragma once
#include "global.h"

class TimerBtn : public QPushButton
{
public:
	TimerBtn(QWidget* parent);
	~TimerBtn();
	void mouseReleaseEvent(QMouseEvent* e) override;                 // 参数e是鼠标事件对象,包含鼠标按键类型、点击位置、事件状态等信息
private:
	QTimer* _timer;
	int _counter;
};

