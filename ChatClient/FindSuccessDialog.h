#pragma once

#include <QDialog>
#include "ui_FindSuccessDialog.h"
#include <memory>
#include "UserData.h"

class FindSuccessDialog : public QDialog
{
	Q_OBJECT

public:
	FindSuccessDialog(QWidget *parent = nullptr);
	~FindSuccessDialog();
	void SetSearchInfo(std::shared_ptr<SearchInfo> si);

private:
	Ui::FindSuccessDialogClass ui;
	QWidget* _parent;                                               // 把parent缓存起来，后期要用,可能会把数据返回给父节点
	std::shared_ptr<SearchInfo> _si;                                // 把SearchInfo缓存起来

private slots:
	void on_add_friend_Button_clicked();
};

