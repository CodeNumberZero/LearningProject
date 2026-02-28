#include "TcpMgr.h"

TcpMgr::TcpMgr() : _host(""), _port(0), _b_rece_pending(false), _message_id(0), _message_len(0)
{
	// 这句虽然看似只有三个参数，但实际上查看源码可以发现它的内部实现是调用常规的四个参数的connect函数版本，默认的信号接收方就是发送发自己
	QObject::connect(&_socket, &QTcpSocket::connected, [&]() {               // connect信号在成功连接到远程主机后触发
		qDebug() << "Connected to server!";
		emit sigConnectSuccess(true);
	});

    // QDataStream是Qt提供的用于二进制数据序列化的类,简单的工作原理:
    // 写入过程:类型T -> QDataStream -> QByteArray
    // 读取过程:QByteArray -> QDataStream -> 类型T
	QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {               // readyRead信号是Qt用来通知应用程序"有新的数据到达,可以读取了"的机制。需要在这个信号的槽函数中，读取并处理接收到的数据(注意:readyRead信号的触发不代表接收到了一个完整的数据包,即可能出现粘包问题)
		// 当有数据可读时，读取所有数据
		// 读取所有数据并追加到缓冲区
		_buffer.append(_socket.readAll());                                   // 在槽函数中，通常使用readAll()函数来读取所有当前可用的数据。它会返回一个QByteArray对象，包含所有尚未读取的字节

        // 这两句存疑，可能要放到循环里
		QDataStream stream(&_buffer, QIODevice::ReadOnly);
		stream.setVersion(QDataStream::Qt_6_9);

        forever{
            //先解析头部
           if (!_b_rece_pending) {
               // 检查缓冲区中的数据是否足够解析出一个消息头（消息ID + 消息长度）
               if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) { // QByteArray的size()函数返回字节序里的总字节数
                   return; // 数据不够，等待更多数据
               }

               // 预读取消息ID和消息长度，但不从缓冲区中移除
               stream >> _message_id >> _message_len;

               //将buffer 中的前四个字节移除
               _buffer = _buffer.mid(sizeof(quint16) * 2);                   // mid接受pos和len两个参数，返回一个新的QByteArray，包含从pos开始的len个字节。pos为起始位置，len为要提取的长度，如果为-1或省略，则提取从pos到末尾的所有字节

               // 输出读取的数据
               qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;

           }

            //buffer剩余长度是否满足消息体长度，不满足则退出继续等待接受
           if (_buffer.size() < _message_len) {
                _b_rece_pending = true;
                return;
           }

           _b_rece_pending = false;
           // 读取消息体
           QByteArray messageBody = _buffer.mid(0, _message_len);
           qDebug() << "Receive body msg is " << messageBody;

           _buffer = _buffer.mid(_message_len);
           HandleMsg(ReqId(_message_id), _message_len, messageBody);
        }
	});

    // 处理错误(适用于5.15之后版本)
    //QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
    //    Q_UNUSED(socketError)
    //    qDebug() << "TcpMgr Error:" << _socket.errorString();
    //});

    // Qt 6 的改进
    connect(&_socket, &QTcpSocket::errorOccurred, [&](QAbstractSocket::SocketError socketError) {
        Q_UNUSED(socketError);
        qDebug() << "TcpMgr Error:" << _socket.errorString();
    });

    // 处理连接断开
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {             // disconnect信号在与远程主机的连接断开后触发    
		qDebug() << "Disconnected from server!";
    });

    // 连接发送信号用来发送数据
    QObject::connect(this, &TcpMgr::sigSendData, this, &TcpMgr::slot_send_data);

    // 注册消息
    initHandlers();
}

TcpMgr::~TcpMgr() {}

void TcpMgr::initHandlers() {
	// auto self = shared_from_this();  
    // 这里不能使用self，不能将self传到lambda表达式中，因为shared_from_this()的使用前提是对象已经构造完成且是使用智能指针进行管理
    // 而initHandlers是在TcpMgr的构造函数中调用的，此时对象还没有完全构造完成，因此在构造函数中shared_from_this()是不可用的，如果在构造函数中调用shared_from_this()，会抛出std::bad_weak_ptr异常。解决方法是在initHandlers中直接使用this指针来捕获当前对象的成员函数，这样就可以避免在构造函数中调用shared_from_this()的问题。
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << ", data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "TcpMgr Login Failed, err is Json Parse Err, error code is " << err;
            emit sigLoginFailed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "TcpMgr Login Failed, err is " << err;
            emit sigLoginFailed(err);
            return;
        }

        //UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        //UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        //UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        emit sigSwitchChat();
    });
}

void TcpMgr::HandleMsg(ReqId id, int len, QByteArray data)
{
    auto it = _handlers.find(id);
    if (it == _handlers.end()) {
        qDebug() << "TcpMgr Not found id [" << id << "] to handle!";
        return;
    }
    // Qt中的QMap和标准库中的map存在区别，前者可以直接通过调用迭代器的value()方法来获取键对应的值，而不需要解引用或者使用->操作符
    it.value()(id, len, data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "TcpMgr receive tcp connect signal";
    // 尝试连接到服务器
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(_host, _port);
}

// 因为客户端发送数据可能在任何线程，为了保证线程安全，我们在要发送数据时发送TcpMgr的sig_send_data信号，然后实现接受这个信号的槽函数slot_send_data，在这个槽函数中进行数据的发送。这样就可以保证数据发送的线程安全性，因为Qt的信号和槽机制会自动处理跨线程的信号传递。
void TcpMgr::slot_send_data(ReqId reqId, QString data)
{
    // QDataStream是Qt提供的用于二进制数据序列化的类,简单的工作原理:
    // 写入过程:类型T -> QDataStream -> QByteArray
    // 读取过程:QByteArray -> QDataStream -> 类型T
    uint16_t id = reqId;

    // 将字符串转换为UTF-8编码的字节数组
    QByteArray dataBytes = data.toUtf8();

    // 计算长度（使用网络字节序转换）
    //quint16 len = static_cast<quint16>(data.size());
    quint16 len = dataBytes.size();

    // 创建一个QByteArray用于存储要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);

    // 写入ID和长度
    out << id << len;

    // 添加字符串数据
    block.append(dataBytes);

    // 发送数据
    _socket.write(block);
}