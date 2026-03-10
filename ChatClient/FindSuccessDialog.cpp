#include "FindSuccessDialog.h"
#include <qdir.h>
#include <qdebug.h>
#include "ApplyFriend.h"

FindSuccessDialog::FindSuccessDialog(QWidget *parent)
	: QDialog(parent), _parent(parent)
{
	ui.setupUi(this);

	setWindowTitle("添加");                                                     // 设置对话框标题
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);             	   // 隐藏对话框标题栏

	// 获取应用程序目录下的图片路径
	/*
		由于资源文件位于源码目录，而代码又是从可执行文件中加载资源文件，所以我们需要设置项目的生成后事件，自动复制一份资源文件到可执行文件目录
		在Visual Studio中配置自动复制static文件夹(因为程序最终的可执行文件.exe会输出到其他目录,所以需要将配置也拷贝到.exe所在的目录;也可以不拷贝,直接将配置文件添加到qrc资源文件中,读取配置时使用qrc文件中的配置就行):
			1、右键项目(ChatClient) -> 选择「属性」
			2、在左侧菜单里找到"生成事件" -> "后期生成事件"
			3、在"命令行"输入框里,粘贴下面的命令:xcopy /Y /E /I "$(ProjectDir)static" "$(TargetDir)static\"
				$(ProjectDir)：表示项目根目录(源码文件所在的位置)
				$(OutDir)：表示编译后的输出目录(Debug/Release,对应Qt Creator的bin目录,即static文件夹复制后存放的目录)
				/Y：覆盖目标文件时不提示确认
				/E：复制目录和子目录，包括空目录
				/I：如果目标不存在，则创建目录
			4、点击"确定"保存设置。
*/
	QString app_path = QCoreApplication::applicationDirPath();                 // 返回应用程序可执行文件所在的目录路径
	QString pix_path = QDir::toNativeSeparators(app_path + QDir::separator() + "static" + QDir::separator() + "head_13.jpg"); // QDir::separator()：返回平台相关的路径分隔符,Windows:\(反斜杠),Linux/macOS:/(正斜杠);QDir::toNativeSeparators()：将路径转换为当前平台的标准格式
	qDebug() << pix_path;
	QPixmap head_pix(pix_path);
	head_pix = head_pix.scaled(ui.head_label->size(),
		Qt::KeepAspectRatio, Qt::SmoothTransformation);
	ui.head_label->setPixmap(head_pix);
	ui.add_friend_Button->SetState("normal", "hover", "press");
	this->setModal(true);                                                      // 设为模态对话框
}

FindSuccessDialog::~FindSuccessDialog()
{}

void FindSuccessDialog::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
	ui.name_label->setText(si->_name);
	_si = si;
}

void FindSuccessDialog::on_add_friend_Button_clicked(){
	this->hide();
	// 弹出加好友界面
	auto applyFriend = new ApplyFriend(_parent);
	applyFriend->SetSearchInfo(_si);
	applyFriend->setModal(true);
	applyFriend->show();
}