#include "ChatPage.h"
#include <qstyleoption.h>

ChatPage::ChatPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    //设置按钮样式
    ui.receive_Button->SetState("normal", "hover", "press");
    ui.send_Button->SetState("normal", "hover", "press");
    //设置图标样式
    ui.emote_label ->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui.file_label->SetState("normal", "hover", "press", "normal", "hover", "press");
}

ChatPage::~ChatPage()
{}

// 如果不重写paintEvent可能无法加载样式表,背景可能不显示，或者显示不正确
void ChatPage::paintEvent(QPaintEvent * event)
{
    QStyleOption opt;                                            // 创建样式选项对象,用于存储绘制控件所需的各种信息（状态、位置、大小等）
    opt.initFrom(this);                                          // 从当前控件初始化选项
    QPainter p(this);                                            // 创建画家对象,用于在控件上绘制,this 指定绘制的目标设备是当前控件
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);   // 绘制背景
}

