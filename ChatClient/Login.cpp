#include "Login.h"
#include "HttpMgr.h"

Login::Login(QWidget *parent)
	: QDialog(parent)
{
    ui.setupUi(this);  // 初始化ui
    ui.err_tip->setProperty("state", "normal");
    repolish(ui.err_tip);
    ui.err_tip->clear(); // 隐藏默认的错误提示

    connect(ui.register_Button, &QPushButton::clicked, this, &Login::sigSwitchRegister);

    ui.forget_label->SetState("normal", "hover", "", "selected", "selected_hover", "");
    ui.forget_label->setCursor(Qt::PointingHandCursor);
    
    connect(ui.forget_label, &ClickedLabel::clicked, this, &Login::slot_forget_pwd);
    initHead();
    initHttpHandlers();

    // 连接登录回包信号和槽函数
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this, &Login::slot_login_mod_finish);
 //   // 连接tcp连接请求的信号和槽函数
 //   connect(this, &Login::sigTcpConnect, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
 //   // 连接tcp管理者发出的连接成功信号
 //   connect(TcpMgr::GetInstance().get(), &TcpMgr::sigConnectSuccess, this, &Login::slot_tcp_connect_finish);
	//// 连接tcp管理者发出的连接失败信号
	//connect(TcpMgr::GetInstance().get(), &TcpMgr::sigLoginFailed, this, &Login::slot_login_failed);
}

Login::~Login()
{
    qDebug() << "Login destruct!";
}

void Login::initHead()
{
    // 加载图片
    QPixmap originalPixmap(":/image/resource/KamneRider.png");
    // 设置图片自动缩放
    qDebug() << originalPixmap.size() << ui.head_label->size();
    originalPixmap = originalPixmap.scaled(ui.head_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建一个和原始图片相同大小的QPixmap，用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent); // 用透明色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿，使圆角更平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); // 最后两个参数分别是x和y方向的圆角半径
    painter.setClipPath(path);

    // 将原始图片绘制到roundedPixmap上
    painter.drawPixmap(0, 0, originalPixmap);

    // 设置绘制好的圆角图片到QLabel上
    ui.head_label->setPixmap(roundedPixmap);
}

void Login::initHttpHandlers() {
    // 注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
			showTip(tr("登陆失败，参数错误，错误码: ") + QString::number(error), false);
            enableBtn(true);
            return;
        }

        auto email = jsonObj["email"].toString();

        // 利用ServerInfo接受信息,然后发送信号通知tcpMgr发送长连接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        // 缓存用户信息(这两句可以不写，后续会做处理)
        _uid = si.Uid;
        _token = si.Token;
        qDebug() << "email is " << email << " \nuid is " << si.Uid << " \nhost is "
            << si.Host << " \nPort is " << si.Port << " \nToken is " << si.Token;
        emit sigTcpConnect(si);
    });
}

bool Login::checkUserValid() {
    auto email = ui.email_lineEdit->text();
    if (email.isEmpty()) {
        //qDebug() << "email empty!";
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool Login::checkPwdValid() {
    auto pwd = ui.pwd_lineEdit->text();
    if (pwd.length() < 6 || pwd.length() > 15) {
        qDebug() << "Pass length invalid";
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pwd).hasMatch();
    if (!match) {
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符且长度为(6~15)"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool Login::enableBtn(bool enabled)
{
    ui.login_Button->setEnabled(enabled);
    ui.register_Button->setEnabled(enabled);
  //  if (!enabled) {
		//showTip(tr("登陆中，请稍候..."), true);
  //  }
    return true;
}

void Login::showTip(QString str, bool b_ok) {
    if (b_ok) {
        ui.err_tip->setProperty("state", "normal");
    }
    else {
        ui.err_tip->setProperty("state", "err");
    }
    ui.err_tip->setText(str);
    repolish(ui.err_tip);
}

void Login::AddTipErr(TipErr te, QString tips) {
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void Login::DelTipErr(TipErr te) {
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {
        ui.err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

void Login::slot_forget_pwd() {
    qDebug() << "slot forget pwd!";
    emit sigSwitchReset();
}

void Login::on_login_Button_clicked() {
    qDebug() << "login btn clicked";
    if (checkUserValid() == false) {
        return;
    }

    if (checkPwdValid() == false) {
        return;
    }

    enableBtn(false);
    auto email = ui.email_lineEdit->text();
    auto pwd = ui.pwd_lineEdit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"), json_obj, ReqId::ID_LOGIN_USER, Modules::LOGIN_MOD);
}

void Login::slot_login_mod_finish(ReqId id, QString result, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {                                    // 先检查请求是否成功
        showTip(tr("登录模块网络请求错误，错误码: ") + QString::number(err), false);
        return;
    }

    // 解析JSON字符串,result转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(result.toUtf8());    // 把后端返回的JSON字符串转换为Qt可操作的JSON文档对象，便于后续提取数据
    if (jsonDoc.isNull()) {
        showTip(tr("登录模块json解析失败"), false);   
        return;
    }

    // json解析失败
    if (!jsonDoc.isObject()) {                                           // 检查解析后的QJsonDocument是否包含一个JSON对象
        showTip(tr("登录模块json解析失败"), false);
        return;
    }

    _handlers[id](jsonDoc.object());                                      // 根据请求ID分发业务逻辑
    return;
}