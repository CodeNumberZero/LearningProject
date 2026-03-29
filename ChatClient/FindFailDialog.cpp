#include "FindFailDialog.h"

FindFailDialog::FindFailDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
    setWindowTitle("添加");                                          // 设置对话框标题
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);        // 隐藏对话框标题栏
    this->setObjectName("FindFailDlg");
    ui.fail_sure_btn->SetState("normal", "hover", "press");
    this->setModal(true);
}

FindFailDialog::~FindFailDialog()
{
    qDebug() << "FindFailDialog destruct!";
}

void FindFailDialog::on_fail_sure_btn_clicked() {
    this->hide();
}