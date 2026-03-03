#pragma once

#include <QDialog>
#include "ui_LoadingDialog.h"

class LoadingDialog : public QDialog
{
	Q_OBJECT

public:
	LoadingDialog(QWidget *parent = nullptr);
	~LoadingDialog();

private:
	Ui::LoadingDialogClass ui;
};

