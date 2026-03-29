#pragma once

#include <QDialog>
#include "ui_FindFailDialog.h"

class FindFailDialog : public QDialog
{
	Q_OBJECT

public:
	FindFailDialog(QWidget *parent = nullptr);
	~FindFailDialog();

private:
	Ui::FindFailDialogClass ui;

private slots:
	void on_fail_sure_btn_clicked();
};

