#pragma once
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>

#include <qabstractsocket.h>
#include <qbytearray.h>
#include <qcryptographichash.h>
#include <qdir.h>
#include <qevent.h>
#include <qfile.h>
#include <qjsonobject.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qpainterpath.h>
#include <qregularexpression.h>
#include <qstyle.h>
#include <qstring.h>
#include <qsettings.h>
#include <qtimer.h>
#include <QtWidgets/QApplication>
#include <qtcpsocket.h>
#include <qwidget.h>

extern QString gate_url_prefix;                          // 前缀

extern std::function<void(QWidget*)> repolish;           // 用来刷新qss
extern std::function<QString(QString)> xorString;        // 使用异或加密密码
extern QString md5Encrypt(const QString& input);         // md5加密

enum ReqId {
    ID_GET_VARIFY_CODE = 1001,                           // 获取验证码
    ID_REG_USER = 1002,                                  // 注册用户
    ID_RESET_PWD = 1003,                                 // 重置密码
    ID_LOGIN_USER = 1004,                                // 用户登录
    ID_CHAT_LOGIN = 1005,                                // 登陆聊天服务器
    ID_CHAT_LOGIN_RSP = 1006,                            // 登陆聊天服务器回包
};

enum Modules {
	REGISTER_MOD = 0,
    RESET_MOD = 1,
    LOGIN_MOD = 2,
};

enum ErrorCodes {
	SUCCESS = 0,
	ERR_JSON = 1,                                        // json解析失败
	ERR_NETWORK = 2,                                     // 网络错误
};

enum TipErr {
    TIP_SUCCESS = 0,                                     // 成功
    TIP_EMAIL_ERR = 1,                                   // 邮箱错误
    TIP_PWD_ERR = 2,                                     // 密码错误
    TIP_AGAIN_ERR = 3,                                   // 确认密码错误 
    TIP_PWD_AGAIN = 4,                                   // 密码和确认密码不匹配                   
    TIP_VARIFY_ERR = 5,                                  // 验证码错误
    TIP_USER_ERR = 6                                     // 用户名错误
};

enum ClickLbState {
    Normal = 0,
    Selected = 1
};

struct ServerInfo
{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

