#include "TimerBtn.h"

TimerBtn::TimerBtn(QWidget* parent) : QPushButton(parent), _counter(10)
{
	_timer = new QTimer(this);
	// 将定时器的timeout()信号(定时器到达间隔时触发)绑定到lambda函数,实现倒计时逻辑;此处未显式设置定时器的间隔setInterval(),需在按钮点击时设置(通常为1000ms,即1秒触发一次timeout())
	connect(_timer, &QTimer::timeout, [this]() {                              
		_counter--;
		if (_counter <= 0) {
			_timer->stop();                                                       // 停止定时器，避免继续触发
			_counter = 10;                                                        // 重置计数器为初始值，为下一次倒计时做准备
			this->setText("获取");                                                 // 恢复按钮默认文本
			this->setEnabled(true);                                               // 启用按钮，允许用户再次点击
			return;
		}
		this->setText(QString::number(_counter) + "s");			                  // 更新按钮文本，显示剩余秒数
	});
}

TimerBtn::~TimerBtn()
{
	_timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent* e)
{
	// 用户松开鼠标左键时，立即启动倒计时，同时向外发送点击信号，通知外部处理后续业务
	if (e->button() == Qt::LeftButton) {                                         // 处理鼠标左键释放事件
		qDebug() << "Left button was released!";
		this->setEnabled(false);                                                 // 鼠标释放后立即禁用按钮,防止用户连续点击
		this->setText(QString::number(_counter) + "s");                          // 实现点击后立即显示倒计时初始秒数
		_timer->start(1000);                                                     // 启动定时器,设置间隔为1000ms(1秒),实现每秒触发一次timeout()
		// _timer->setInterval(1000);                                            // 另一种写法
		// _timer->start();
		emit clicked();                                                          // 发送按钮点击信号，通知外部处理后
	}

	QPushButton::mouseReleaseEvent(e);                                           // 调用父类的事件处理函数:1、确保其他鼠标释放逻辑正常执行(保留QPushButton原生的点击视觉效果,如按钮按下后的弹起、hover/clicked样式变化,否则按钮点击后无视觉反馈，交互体验差) 2、确保父类对鼠标事件的其他默认处理逻辑正常执行，避免事件处理不完整导致的控件异常
}
