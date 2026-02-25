#include "HttpMgr.h"

HttpMgr::~HttpMgr()
{
}

HttpMgr::HttpMgr()
{
	connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
	QByteArray data = QJsonDocument(json).toJson();                                   // 将json对象转换为HTTP请求可传输的字节数组(QJsonDocument是Qt中用于封装JSON数据的容器，可实现JSON数据结构与字节流的相互转换；toJson()将QJsonDocument序列化为JSON格式的字节数组)
	QNetworkRequest request(url);                                                     // 初始化请求对象，指定请求的目标URL 
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");        // 设置请求头，告诉后端服务器本次请求的请求体数据格式是JSON格式
	request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));  // 设置请求头，告诉后端服务器本次请求体的字节大小
	auto self = shared_from_this();
	QNetworkReply* reply = _manager.post(request, data);                              // 发送 HTTP POST 请求,并返回后端的回复信息
	QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod]() {  // 使用Qt的信号与槽机制，绑定请求完成信号与回调函数(QNetworkReply的finished信号是请求完成时触发，无论成功或失败)
		// 处理错误情况
		if (reply->error() != QNetworkReply::NoError) {
			qDebug() << reply->errorString();
			// 发送信号通知成功
			emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);     // emit是Qt框架中的一个关键字，专门用来触发(发送)信号
			reply->deleteLater();
			return;
		}

		// 无错误
		QString result = reply->readAll();
		// 发送信号通知成功
		emit self->sig_http_finish(req_id, result, ErrorCodes::SUCCESS, mod);
		reply->deleteLater();
		return;
	});
}

void HttpMgr::slot_http_finish(ReqId id, QString result, ErrorCodes err, Modules mod) {
	if (mod == Modules::REGISTER_MOD) {
		// 发送信号通知指定模块的http的响应结束了
		emit sig_reg_mod_finish(id, result, err);                                     // 将注册模块的消息发送到注册界面
	}
	if (mod == Modules::RESET_MOD) {
		emit sig_reset_mod_finish(id, result, err);                                   // 将重置模块的消息发送到重置界面
	}
	if (mod == Modules::LOGIN_MOD) {
		emit sig_login_mod_finish(id, result, err);                                   // 将登录模块的消息发送到登录界面
	}
}
