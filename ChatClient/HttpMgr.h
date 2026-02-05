#pragma once
#include "Singleton.h"
#include <qstring.h>
#include <qurl.h>
#include <qobject.h>
#include <qnetworkaccessmanager.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qnetworkreply.h>

// 继承QObject以便于可以使用信号和槽
// CRTP:奇异递归模板
class HttpMgr : public QObject, public Singleton<HttpMgr>, public std::enable_shared_from_this<HttpMgr> 
{
	Q_OBJECT
public:
	//析构设为公有的原因：单例基类中有个成员变量_instance，在析构时会回收这个变量，而这个变量又是个智能指针，指针指向子类对象，所以回收变量就是析构智能指针，析构智能指针又要析构其指向的子类对象，而析构子类对象要调用子类的析构函数，如果子类的析构设为私有就无法调用了
	~HttpMgr();
	void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);     // 发送http的post请求，用于向指定URL发送JSON格式的请求数据

private:
	friend class Singleton<HttpMgr>;                                             // 声明友元类(因为基类中创建实例时使用了new开辟子类指针，需要访问子类的构造函数)
	HttpMgr();
	QNetworkAccessManager _manager;                                              // Qt原生的http网络管理者(Qt中用于管理网络请求的核心类)

private slots:
	void slot_http_finish(ReqId id, QString result, ErrorCodes err, Modules mod);

signals:
	void sig_http_finish(ReqId id, QString result, ErrorCodes err, Modules mod);
	void sig_reg_mod_finish(ReqId id, QString result, ErrorCodes err);
	void sig_reset_mod_finish(ReqId id, QString result, ErrorCodes err);
};

