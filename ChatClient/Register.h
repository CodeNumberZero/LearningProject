#pragma once

#include <QDialog>
#include "ui_Register.h"
#include "global.h"

class Register : public QDialog
{
	Q_OBJECT

public:
	Register(QWidget *parent = nullptr);
	~Register();

private slots:
	void on_getcode_Button_clicked();
	void on_comfirm_Button_clicked();
	void on_cancel_Button_clicked();
	void slot_reg_mod_finish(ReqId id, QString result, ErrorCodes err);
	void on_return_Button_clicked();

signals:
	void sigSwitchLogin();

private:
	Ui::RegisterClass ui;
	void showTip(QString str, bool b_ok);
	void AddTipErr(TipErr te, QString tips);                                         // 向_tip_errs中增加错误提示，包括TipErr和相应的文本
	void DelTipErr(TipErr te);                                                       // 从_tip_errs中删掉指定错误提示
	void initHttpHandlers();
	QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

	QMap<TipErr, QString> _tip_errs;                                                 // 存储错误提示(键为TipErr，值为相应的错误提示文本)
	bool checkUserValid();
	bool checkEmailValid();
	bool checkPassValid();
	bool checkAgainValid();
	bool checkVarifyValid();

	QTimer* _countdown_timer;
	int _countdown;
	void ChangeTipPage();                                                            // 页面切换
};

