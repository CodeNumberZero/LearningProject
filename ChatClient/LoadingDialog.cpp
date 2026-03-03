#include "LoadingDialog.h"
#include <qmovie.h>

LoadingDialog::LoadingDialog(QWidget *parent)
	: QDialog(parent)
{
	/*
	传递this指针可以设置父对象关系,创建的控件都以 LoadingDialog 为父对象
	这样做的目的是：
	   - 控件会显示在 LoadingDialog 上
	   - 内存管理：当 LoadingDialog 销毁时，子控件自动销毁
	   - 事件传递：事件可以正确传递给父对象
	*/
	ui.setupUi(this);                                         //  将 ui 文件中的界面设置到 this 对象上,之后可以访问界面上的控件(将设计师设计的界面(.ui 文件)实例化到当前对象上)

	setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint); // 设置一个无边框(即不显示菜单)、带系统菜单、始终置顶的对话框(注意：Qt::WindowSystemMenuHint与 Qt::FramelessWindowHint 同时使用时，前者可能被忽略)
	setAttribute(Qt::WA_TranslucentBackground);               // 设置背景透明
	// 获取屏幕尺寸
	setFixedSize(parent->size());                             // 设置对话框为全屏尺寸

	QMovie* movie = new QMovie(":/gif/resource/loading.gif"); // 加载动画的资源文件
	ui.loading_label->setMovie(movie);
	movie->start();
}

LoadingDialog::~LoadingDialog()
{}

