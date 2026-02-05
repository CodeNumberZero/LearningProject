#include "Register.h"
#include "global.h"
#include "HttpMgr.h"

Register::Register(QWidget *parent)
	: QDialog(parent), _countdown(5)
{
	ui.setupUi(this);

	// 设置密码格式隐藏
	ui.pwd_lineEdit->setEchoMode(QLineEdit::Password);
	ui.again_lineEdit->setEchoMode(QLineEdit::Password);
	ui.err_tip->setProperty("state", "normal");
	repolish(ui.err_tip);
	connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, 
		this, &Register::slot_reg_mod_finish);

	// 注册相应的回调函数
	initHttpHandlers();
	ui.err_tip->clear();

	// 检查注册用户时的书写是否合法
	connect(ui.user_lineEdit, &QLineEdit::editingFinished, this, [this]() {      // 槽函数使用lambda函数的原因：写lambda的话函数名写错了就会报错，直接写函数名那错了就很难知道了
		checkUserValid();
	});
	connect(ui.email_lineEdit, &QLineEdit::editingFinished, this, [this]() {
		checkEmailValid();
	});
	connect(ui.pwd_lineEdit, &QLineEdit::editingFinished, this, [this]() {
		checkPassValid();
	});
	connect(ui.again_lineEdit, &QLineEdit::editingFinished, this, [this]() {
		checkAgainValid();
	});
	connect(ui.varify_lineEdit, &QLineEdit::editingFinished, this, [this]() {
		checkVarifyValid();
	});

	//设置浮动显示手形状
	ui.pwd_visible->setCursor(Qt::PointingHandCursor);
	ui.again_visible->setCursor(Qt::PointingHandCursor);

	ui.pwd_visible->SetState("unvisible", "unvisible_hover", "", "visible",
		"visible_hover", "");
	ui.again_visible->SetState("unvisible", "unvisible_hover", "", "visible",
		"visible_hover", "");

	//连接信号和槽，实现点击切换
	connect(ui.pwd_visible, &ClickedLabel::clicked, this, [this]() {
		auto state = ui.pwd_visible->GetCurState();
		if (state == ClickLbState::Normal) {
			ui.pwd_lineEdit ->setEchoMode(QLineEdit::Password);
		}
		else {
			ui.pwd_lineEdit->setEchoMode(QLineEdit::Normal);
		}
		qDebug() << "Label was clicked!";
	});
	connect(ui.again_visible, &ClickedLabel::clicked, this, [this]() {
		auto state = ui.again_visible->GetCurState();
		if (state == ClickLbState::Normal) {
			ui.again_lineEdit->setEchoMode(QLineEdit::Password);
		}
		else {
			ui.again_lineEdit->setEchoMode(QLineEdit::Normal);
		}
		qDebug() << "Label was clicked!";
	});

	// 创建定时器
	_countdown_timer = new QTimer(this);
	// 连接信号和槽
	connect(_countdown_timer, &QTimer::timeout, [this]() {
		if (_countdown == 0) {
			_countdown_timer->stop();
			emit sigSwitchLogin();
			return;
		}
		_countdown--;
		auto str = QString("注册成功，%1 s后返回登录页面").arg(_countdown);
		ui.tip1_label->setText(str);
	});
}

Register::~Register()
{
	qDebug() << "Register destruct!";
}

void Register::on_comfirm_Button_clicked()
{
	// 检测所有条件成立后再发送请求
	bool valid = checkUserValid();
	if (!valid) {
		return;
	}

	valid = checkEmailValid();
	if (!valid) {
		return;
	}

	valid = checkPassValid();
	if (!valid) {
		return;
	}

	valid = checkAgainValid();
	if (!valid) {
		return;
	}

	valid = checkVarifyValid();
	if (!valid) {
		return;
	}

	// 发送http请求注册用户
	QJsonObject json_obj;
	json_obj["user"] = ui.user_lineEdit->text();
	json_obj["email"] = ui.email_lineEdit->text();
	json_obj["passwd"] = xorString(ui.pwd_lineEdit->text());
	json_obj["confirm"] = xorString(ui.again_lineEdit->text());
	json_obj["varifycode"] = ui.varify_lineEdit->text();
	HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_register"), json_obj, ReqId::ID_REG_USER, Modules::REGISTER_MOD);
}

void Register::on_cancel_Button_clicked()
{
	_countdown_timer->stop();
	emit sigSwitchLogin();
}

void Register::on_getcode_Button_clicked() {
	auto email = ui.email_lineEdit->text();
	QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
	bool match = regex.match(email).hasMatch();
	if (match) {
		// 发送http验证码
		QJsonObject json_obj;
		json_obj["email"] = email;
		HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"), json_obj, ReqId::ID_GET_VARIFY_CODE, Modules::REGISTER_MOD);
	}
	else {
		/*
			1、Visual Studio默认编码：Windows下通常是GBK/GB2312，而Qt默认使用UTF-8编码
			2、当在代码中直接写中文(如tr("邮箱地址不正确"))时，若源文件是GBK编码，Qt的tr()函数会把GBK编码的字符串当作UTF-8解析，导致乱码;这时就需要使用QString::fromLocal8Bit()显式转码，将GBK编码的字符串转为UTF-8
			3、QString::fromLocal8Bit()是Qt中用于将本地编码(如Windows下的GBK/GB2312、Linux下的UTF-8)的字节数组转换为QString(UTF-16编码)的核心函数，专门解决不同编码间的字符串转换问题
			4、两种方式二选一，不需要多语言翻译时选第一种，需要支持多语言时选第二种
		*/
		showTip(tr("邮箱地址不正确"), false);
		//ShowTip(tr(QString::fromLocal8Bit("邮箱地址不正确").toStdString().c_str()));
	}
}

void Register::slot_reg_mod_finish(ReqId id, QString result, ErrorCodes err)
{
	if (err != ErrorCodes::SUCCESS) {                                    // 先检查请求是否成功
		showTip(tr("网络请求错误"), false);
		return;
	}

	// 解析JSON字符串,result转化为QByteArray
	QJsonDocument jsonDoc = QJsonDocument::fromJson(result.toUtf8());    // 把后端返回的JSON字符串转换为Qt可操作的JSON文档对象，便于后续提取数据
	if (jsonDoc.isNull()) {
		showTip(tr("json解析失败"), false);   // 用中文就会报错，不知道是为什么
		return;
	}

	// json解析失败
	if (!jsonDoc.isObject()) {                                           // 检查解析后的QJsonDocument是否包含一个JSON对象
		showTip(tr("json解析失败"), false);
		return;
	}

	_handlers[id](jsonDoc.object());                                      // 根据请求ID分发业务逻辑
	return;
}

void Register::on_return_Button_clicked()
{
	_countdown_timer->stop();
	emit sigSwitchLogin();
}

void Register::initHttpHandlers()
{
	// 注册获取验证码回包的逻辑
	_handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj){
		int error = jsonObj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			showTip(tr("参数错误"), false);
			return;
		}

		auto email = jsonObj["email"].toString();
		showTip(tr("验证码已发送到邮箱，注意查收"), true);
		qDebug() << "email is" << email;
	});

	// 注册注册用户回包的逻辑
	_handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj) {
		int error = jsonObj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			showTip(tr("参数错误"), false);
			return;
		}

		auto email = jsonObj["email"].toString();
		showTip(tr("用户注册成功"), true);
		qDebug() << "user uuid is " << jsonObj["uuid"].toString();
		qDebug() << "email is" << email;
		ChangeTipPage();                                                           // 收到服务器注册成功的回复后切换界面
	});
}

bool Register::checkUserValid()
{
	if (ui.user_lineEdit->text() == "") {
		AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
		return false;
	}
	DelTipErr(TipErr::TIP_USER_ERR);
	return true;
}

bool Register::checkPassValid()
{
	auto pass = ui.pwd_lineEdit->text();
	auto again = ui.again_lineEdit->text();

	if (pass.length() < 6 || pass.length() > 15) {
		//提示长度不准确
		AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
		return false;
	}
	// 创建一个正则表达式对象，按照上述密码要求
	// 这个正则表达式解释：
	// ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
	QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
	bool match = regExp.match(pass).hasMatch();
	if (!match) {
		//提示字符非法
		AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度可以是字母、数字和特定的特殊字符,不能包含非法字符"));
		return false;;
	}
	DelTipErr(TipErr::TIP_PWD_ERR);

	if (pass != again) {
		//提示密码不匹配
		AddTipErr(TipErr::TIP_PWD_AGAIN, tr("密码和确认密码不匹配"));
		return false;
	}
	else {
		DelTipErr(TipErr::TIP_PWD_AGAIN);
	}

	return true;
}

bool Register::checkAgainValid()
{
	auto pass = ui.pwd_lineEdit->text();
	auto again = ui.again_lineEdit->text();

	if (again.length() < 6 || again.length() > 15) {
		//提示长度不准确
		AddTipErr(TipErr::TIP_AGAIN_ERR, tr("密码长度应为6~15"));
		return false;
	}

	// 创建一个正则表达式对象，按照上述密码要求
	// 这个正则表达式解释：
	// ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
	QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
	bool match = regExp.match(again).hasMatch();
	if (!match) {
		//提示字符非法
		AddTipErr(TipErr::TIP_AGAIN_ERR, tr("不能包含非法字符"));
		return false;
	}

	DelTipErr(TipErr::TIP_AGAIN_ERR);

	if (pass != again) {
		//提示密码不匹配
		AddTipErr(TipErr::TIP_PWD_AGAIN, tr("确认密码和密码不匹配"));
		return false;
	}
	else {
		DelTipErr(TipErr::TIP_PWD_AGAIN);
	}
	return true;
}

bool Register::checkEmailValid()
{
	//验证邮箱的地址正则表达式
	auto email = ui.email_lineEdit->text();
	// 邮箱地址的正则表达式
	QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
	bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
	if (!match) {
		//提示邮箱不正确
		AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
		return false;
	}
	DelTipErr(TipErr::TIP_EMAIL_ERR);
	return true;
}

bool Register::checkVarifyValid()
{
	auto pass = ui.varify_lineEdit->text();
	if (pass.isEmpty()) {
		AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
		return false;
	}
	DelTipErr(TipErr::TIP_VARIFY_ERR);
	return true;
}

void Register::showTip(QString str, bool b_ok)
{
	if (b_ok) {
		ui.err_tip->setProperty("state", "normal");
	}
	else {
		ui.err_tip->setProperty("state", "err");
	}
	ui.err_tip->setText(str);
	repolish(ui.err_tip);
}

void Register::AddTipErr(TipErr te, QString tips)
{
	_tip_errs[te] = tips;
	showTip(tips, false);
}

void Register::DelTipErr(TipErr te)
{
	_tip_errs.remove(te);
	if (_tip_errs.empty()) {
		ui.err_tip->clear();
		return;
	}
	showTip(_tip_errs.first(), false);
}

void Register::ChangeTipPage()
{
	_countdown_timer->stop();
	ui.stackedWidget->setCurrentWidget(ui.page_2);
	// 启动定时器，设置间隔为1000毫秒（1秒）
	_countdown_timer->start(1000);
}