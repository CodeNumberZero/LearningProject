#include "MainWindow.h"
#include "TcpMgr.h"
#include "qmessagebox.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    /*
    如果希望Login是主窗口内的控件（而非独立窗口），需去掉_login->show()，仅通过setCentralWidget(_login)将其嵌入主窗口，此时主窗口的图标会正常显示；
    如果通过_login->show()单独显示了Login对话框（它是独立窗口），此时屏幕上显示的是Login的窗口，而非MainWindow的窗口，所以MainWindow的图标不会显示,需要单独为Login窗口设置图标。
    */
    ui.setupUi(this);

    // 创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login = new Login(this);
    _login->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login);
    _login->setWindowIcon(QIcon(":/icon/resource/KamenRider.ico"));
    //_login->show();

    // 连接登录界面注册信号
    connect(_login, &Login::sigSwitchRegister, this, &MainWindow::SlotSwitchReg);

    // 连接登录界面忘记密码信号
    connect(_login, &Login::sigSwitchReset, this, &MainWindow::SlotSwitchReset);

    // 连接创建聊天界面信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigSwitchChat, this, &MainWindow::SlotSwitchChat);

    // 连接服务器踢人消息
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sigNotifyOffline, this, &MainWindow::SlotOffLine);

    setWindowIcon(QIcon(":/icon/resource/KamenRider.ico")); // :/是Qt资源文件的固定前缀。若想直接使用本地磁盘文件不嵌入资源，需传入完整本地路径

    //emit TcpMgr::GetInstance()->sigSwitchChat();
}


MainWindow::~MainWindow()
{
}

void MainWindow::SlotSwitchReg() {
    // 注册界面不在默认构造初始化是为了动态初始化，这样每次切换到注册界面就构造，切换到其它界面就自动回收注册界面
    _register = new Register(this);
    _register->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    //_register->hide();            // 这一句可有可无

    // 连接注册界面返回登录信号
    connect(_register, &Register::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_register);
    _login->hide();
    _register->show();
}

void MainWindow::SlotSwitchLogin()
{
    // 创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login = new Login(this);
    _login->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login);
    _register->hide();
    _login->show();
    // 连接登录界面注册信号
    connect(_login, &Login::sigSwitchRegister, this, &MainWindow::SlotSwitchReg);
    // 连接登录界面忘记密码信号
    connect(_login, &Login::sigSwitchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    // 创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _reset = new ResetDialog(this);
    _reset->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_reset);
    _login->hide();
    _reset->show();
    // 注册返回登录信号和槽函数
    connect(_reset, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLoginFromReset);
}

void MainWindow::SlotSwitchLoginFromReset()
{
    // 创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login = new Login(this);
    _login->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login);
    _reset->hide();
    _login->show();
    // 连接登录界面注册信号
    connect(_login, &Login::sigSwitchRegister, this, &MainWindow::SlotSwitchReg);
    // 连接登录界面忘记密码信号
    connect(_login, &Login::sigSwitchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchChat()
{
    _chat = new Chat(this);
    _chat->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_chat);
    _chat->show();
    _login->hide();
    this->setMinimumSize(QSize(1050, 900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void MainWindow::SlotOffLine()
{
    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提醒", "该账号异地登录,本设备下线!");
    TcpMgr::GetInstance()->CloseConnection();                   // 关闭网络连接
    OffLineLogin();
}

// 执行下线后的处理逻辑(通常是返回到登录界面)
void MainWindow::OffLineLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login = new Login(this);
    _login->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login);

    _chat->hide();
    this->setMaximumSize(400, 550);
    this->setMinimumSize(400, 550);
    this->resize(400, 550);                                                             // 切换到登录界面后调整窗口大小
    _login->show();
    connect(_login, &Login::sigSwitchRegister, this, &MainWindow::SlotSwitchReg);       // 连接登录界面注册信号
    connect(_login, &Login::sigSwitchReset, this, &MainWindow::SlotSwitchReset);        // 连接登录界面忘记密码信号
}
