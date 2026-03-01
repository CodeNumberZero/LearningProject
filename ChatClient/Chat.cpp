#include "Chat.h"

Chat::Chat(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	ui.add_Button->SetState("normal", "hover", "press");             // 因为在ClickedButton的构造函数中没有进行初始化，所以这里一定要显式的设置一下ClickedButton的三种状态
}

Chat::~Chat()
{}

