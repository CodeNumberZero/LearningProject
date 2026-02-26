#pragma once

#include <QDialog>
#include "ui_Login.h"
#include "global.h"

class Login : public QDialog
{
	Q_OBJECT

public:
	Login(QWidget *parent = nullptr);
	~Login();

private:
	Ui::LoginClass ui;
	void initHead();                                                                 // 初始化登录界面头像
	void initHttpHandlers();                                                         // 注册相应的回调函数
	bool checkUserValid();
	bool checkPwdValid();
	bool enableBtn(bool enabled);                                                    // 正在处理登录/注册操作时禁用登录/注册按钮，避免重复点击
	void showTip(QString str, bool b_ok);
	void AddTipErr(TipErr te, QString tips);                                         // 向_tip_errs中增加错误提示，包括TipErr和相应的文本
	void DelTipErr(TipErr te);                                                       // 从_tip_errs中删掉指定错误提示

	QMap<TipErr, QString> _tip_errs;                                                 // 存储错误提示(键为TipErr，值为相应的错误提示文本)
	QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;                  // 存储回调函数(键为请求id，值为相应的回调函数)
	int _uid;
	QString _token;
signals:
	void sigSwitchRegister();
	void sigSwitchReset();
	void sigTcpConnect(ServerInfo);

private slots:
	void slot_forget_pwd();
	void on_login_Button_clicked();
	void slot_login_mod_finish(ReqId id, QString result, ErrorCodes err);
	void slot_tcp_connect_finish(bool b_success);
	void slot_login_failed(int err);
};

