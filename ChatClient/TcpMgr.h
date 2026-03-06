#pragma once
#include "global.h"
#include "Singleton.h"
#include "UserData.h"

// 客户端TCP管理类，用来管理TCP连接
class TcpMgr : public QObject, public Singleton<TcpMgr>, public std::enable_shared_from_this<TcpMgr>
{
	Q_OBJECT
public:
	~TcpMgr();
private:
	QTcpSocket _socket;
	QString _host;
	uint16_t _port;
	QByteArray _buffer;
	bool _b_rece_pending;
	quint16 _message_id;
	quint16 _message_len;
	QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

	friend class Singleton<TcpMgr>;
	TcpMgr();
	void initHandlers();
	void HandleMsg(ReqId id, int len, QByteArray data);
public slots:
	void slot_tcp_connect(ServerInfo si);
	void slot_send_data(ReqId id, QString data);
signals:
	void sigConnectSuccess(bool b_success);
	void sigSendData(ReqId id, QString data);
	void sigSwitchChat();
	void sigLoginFailed(int err);
	void sigUserSearch(std::shared_ptr<SearchInfo> si);
};

