#pragma once

#include <QDialog>
#include "ui_Login.h"

class Login : public QDialog
{
	Q_OBJECT

public:
	Login(QWidget *parent = nullptr);
	~Login();

private:
	Ui::LoginClass ui;

signals:
	void sigSwitchRegister();
	void sigSwitchReset();

private slots:
	void slot_forget_pwd();
};

