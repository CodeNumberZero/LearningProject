#pragma once
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>

#include <qabstractsocket.h>
#include <qaction.h>
#include <qbytearray.h>
#include <qcryptographichash.h>
#include <qdir.h>
#include <qevent.h>
#include <qfile.h>
#include <qframe.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qpainterpath.h>
#include <qrandom.h>
#include <qregularexpression.h>
#include <qstyle.h>
#include <qstring.h>
#include <qsettings.h>
#include <qscrollbar.h>
#include <qtimer.h>
#include <qtextedit.h>
#include <QtWidgets/QApplication>
#include <qtcpsocket.h>
#include <qwidget.h>

extern QString gate_url_prefix;                          // 前缀

extern std::function<void(QWidget*)> repolish;           // 用来刷新qss
extern std::function<QString(QString)> xorString;        // 使用异或加密密码
extern QString md5Encrypt(const QString& input);         // md5加密

// 申请好友标签输入框最低长度
const int MIN_APPLY_LABEL_ED_LEN = 40;

const QString add_prefix = "添加标签 ";

const int tip_offset = 5;

const int CHAT_COUNT_PER_PAGE = 13;                      // 列表每一页的显示数量


/*
    定义一些全局的变量用来做测试；
    1、这些变量如果放到global.h中，必须加extern关键字表明这是声明，然后在global.cpp中给出具体定义.
        因为如果不加extern,即使不初始化，容器也会调用默认构造，分配内存形成定义，又因为globla.h在多个文件中被包含，变量放进去可能会出现重定义的问题；
        例如BubbleFrame.h和TextBubble.h都包含了global.h，而TextBubble.h又包含了BubbleFrame.h，这种出现了TextBubble.h包含两次global.h,导致在链接时出错
    2、另一种方法：如果想在global.h中完成初始化又不想出现重定义问题，只需要在每个变量前添加inline关键字即可,如下：
        inline const std::vector<QString>  strs = {...};
        inline const std::vector<QString> heads = {...};
        inline const std::vector<QString> names = {...};
*/
//extern const std::vector<QString>  strs;
//extern const std::vector<QString> heads;
//extern const std::vector<QString> names;

inline const std::vector<QString>  strs = { "hello world !",
                             "nice to meet u",
                             "New year，new life",
                            "You have to love yourself",
                            "My love is written in the wind ever since the whole world is you" };

inline const std::vector<QString> heads = {
    ":/image/resource/head_1.jpg",
    ":/image/resource/head_6.jpg",
    ":/image/resource/head_7.jpg",
    ":/image/resource/head_19.jpg",
    ":/image/resource/head_6.jpg"
};

inline const std::vector<QString> names = {
    "zero-one",
    "saber",
    "revice",
    "geat",
    "gavv",
    "zzz",
    "python",
    "rust"
};

enum ReqId {
    ID_GET_VARIFY_CODE = 1001,                              // 获取验证码
    ID_REG_USER = 1002,                                     // 注册用户
    ID_RESET_PWD = 1003,                                    // 重置密码
    ID_LOGIN_USER = 1004,                                   // 用户登录
    ID_CHAT_LOGIN = 1005,                                   // 登陆聊天服务器
    ID_CHAT_LOGIN_RSP = 1006,                               // 登陆聊天服务器回包
    ID_SEARCH_USER_REQ = 1007,                              // 用户搜索请求
    ID_SEARCH_USER_RSP = 1008,                              // 搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,                               // 添加好友申请
    ID_ADD_FRIEND_RSP = 1010,                               // 申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,                        // 通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 1013,                              // 认证好友请求
    ID_AUTH_FRIEND_RSP = 1014,                              // 认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,                       // 通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ = 1017,                            // 文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP = 1018,                            // 文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,                     // 通知用户文本聊天信息
    ID_NOTIFY_OFF_LINE_REQ = 1021,                          // 通知用户下线
    ID_HEART_BEAT_REQ = 1023,                               // 心跳请求
    ID_HEARTBEAT_RSP = 1024,                                // 心跳回复
    ID_LOAD_CHAT_THREAD_REQ = 1025,                         // 加载聊天线程
    ID_LOAD_CHAT_THREAD_RSP = 1026,                         // 加载聊天线程回复
    ID_CREATE_PRIVATE_CHAT_REQ = 1027,                      // 创建私聊请求
    ID_CREATE_PRIVATE_CHAT_RSP = 1028,                      // 创建私聊回复
    ID_LOAD_CHAT_MSG_REQ = 1029,                            // 加载聊天消息
    ID_LOAD_CHAT_MSG_RSP = 1030,                            // 加载聊天消息
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

// 可点击Label的两种状态
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

// 聊天界面的几种模式
enum ChatUIMode {
    SearchMode,                                          // 搜索模式
    ChatMode,                                            // 聊天模式
    ContactMode,                                         // 联系人模式
};

// 自定义QListWidgetItem的几种类型
enum ListItemType {
    CHAT_USER_ITEM,                                      // 聊天用户
    CONTACT_USER_ITEM,                                   // 联系人用户
    SEARCH_USER_ITEM,                                    // 搜索到的用户
    ADD_USER_TIP_ITEM,                                   // 提示添加用户
    INVALID_ITEM,                                        // 不可点击条目
    GROUP_TIP_ITEM,                                      // 分组提示条目
    LINE_ITEM,                                           // 分割线
    APPLY_FRIEND_ITEM,                                   // 好友申请
};

// 聊天角色
enum class ChatRole
{
    Self,
    Other
};

// 消息类型
struct MsgInfo {
    QString msgFlag;                                     // "text,image,file"
    QString content;                                     // 表示文件和图像的url,文本信息
    QPixmap pixmap;                                      // 文件和图片的缩略图
};

