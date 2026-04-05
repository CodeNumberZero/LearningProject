#include "TcpMgr.h"
#include "UserMgr.h"

TcpMgr::TcpMgr() : _host(""), _port(0), _b_rece_pending(false), _message_id(0), _message_len(0)
{
	// 这句虽然看似只有三个参数，但实际上查看源码可以发现它的内部实现是调用常规的四个参数的connect函数版本，默认的信号接收方就是发送发自己
	QObject::connect(&_socket, &QTcpSocket::connected, [&]() {               // connect信号在成功连接到远程主机后触发
		qDebug() << "Connected to server success!";
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
    
    // 对应ChatServer中LogicSystem.cpp文件LoginHandler方法中发送的MSG_CHAT_LOGIN_RSP的回包信号(待解决:为什么信号不相同？)
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << ", data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr's ID_CHAT_LOGIN_RSP Failed to create QJsonDocument.";
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

        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        //auto desc = jsonObj["desc"].toString();
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex);
        UserMgr::GetInstance()->SetUserInfo(user_info);
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());

        // 加载好友申请列表(自己的想法:这里可以发送一个信号通知客户端显示红点)
        if (jsonObj.contains("apply_list")) {      // 对应ChatServer中LogicSystem.cpp文件中第129行
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }

        // 加载好友列表
        if (jsonObj.contains("friend_list")) {    // 对应ChatServer中LogicSystem.cpp文件中第145行
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        emit sigSwitchChat();
    });

    // 当我们发送数据后服务器会处理，返回ID_SEARCH_USER_RSP包，所以客户端要实现对ID_SEARCH_USER_RSP包的处理
    // 对应ChatServer中LogicSystem.cpp文件SearchInfo方法中发送的ID_SEARCH_USER_RSP的回包信号
    _handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr's ID_SEARCH_USER_RSP Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {                                          // 验证服务器返回的JSON数据结构是否符合预期;如果服务器返回的数据不符合协议,缺少必要的 error 字段
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "TcpMgr Search User Failed, err is Json Parse Err, error code is" << err;
            emit sigUserSearch(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "TcpMgr Search User Failed, err is " << err;
            emit sigUserSearch(nullptr);
            return;
        }

        auto search_info = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(), jsonObj["name"].toString(),
                                                        jsonObj["nick"].toString(), jsonObj["desc"].toString(),
                                                        jsonObj["sex"].toInt(), jsonObj["icon"].toString());

        emit sigUserSearch(search_info);
    });

    // 对应ChatServer中LogicSystem.cpp文件AddFriendApply方法中发送的ID_ADD_FRIEND_RSP的回包信号
    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr's ID_ADD_FRIEND_RSP Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "TcpMgr Add Friend Failed, err is Json Parse Err, error code is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "TcpMgr Add Friend Failed, err is " << err;
            return;
        }

        qDebug() << "TcpMgr Add Friend Success ";
    });

    // 一个客户端发送申请后,另一个客户端会收到服务器通知添加好友的请求,所以在TcpMgr里监听这个请求
    // 对应ChatServer中LogicSystem.cpp文件AddFriendApply方法中发送的ID_NOTIFY_ADD_FRIEND_REQ信号以及ChatServer中ChatServiceImpl.cpp文件NotifyAddFriend方法中发送的ID_NOTIFY_ADD_FRIEND_REQ
    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr's ID_NOTIFY_ADD_FRIEND_REQ Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "TcpMgr Notify Add Friend Failed, err is Json Parse Err, error code is " << err;
            emit sigUserSearch(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "TcpMgr Notify Add Friend Failed, err is " << err;
            emit sigUserSearch(nullptr);
            return;
        }

        // 收到回包后,获取相应信息(以便得知是谁发送的申请),在己方客户端进行展示
        int from_uid = jsonObj["applyuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString desc = jsonObj["desc"].toString();
        QString icon = jsonObj["icon"].toString();
        QString nick = jsonObj["nick"].toString();
        int sex = jsonObj["sex"].toInt();

        auto apply_info = std::make_shared<AddFriendApply>(
            from_uid, name, desc,
            icon, nick, sex);

        emit sigFriendApply(apply_info);
    });

	// A向B发出好友申请后，需要B在客户端进行好友认证,B完成认证后再发出sigAddFriendAuth信号通知A客户端刷新相关界面
    // (服务器将消息转发给B，B收到服务器的通知后会触发ID_NOTIFY_AUTH_FRIEND_REQ的处理函数，在这个函数中解析服务器发送的数据，并将好友申请的信息封装成AuthInfo对象，然后通过sigAddFriendAuth信号将这个对象发送给UI层，UI层接收到这个信号后就可以在界面上显示好友认证的相关信息了)
    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr's ID_NOTIFY_AUTH_FRIEND_REQ Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "TcpMgr Notify Authen Friend Failed, err is Json Parse Err, error code is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "TcpMgr Notify Authen Friend Failed, err is " << err;
            return;
        }

        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString nick = jsonObj["nick"].toString();
        QString icon = jsonObj["icon"].toString();
        int sex = jsonObj["sex"].toInt();

        auto auth_info = std::make_shared<AuthInfo>(from_uid, name, nick, icon, sex);

        emit sigAddFriendAuth(auth_info);
    });

    // B处理了A发过来的好友申请后(完成了好友认证后,即点击确认同意他人的申请后)，发送sigAuthRsp信号实现B客户端相关界面的刷新
    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "TcpMgr's ID_AUTH_FRIEND_RSP Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "TcpMgr Authen Friend Failed, err is Json Parse Err, error code is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "TcpMgr Authen Friend Failed, err is " << err;
            return;
        }

        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
        emit sigAuthRsp(rsp);

        qDebug() << "Auth Friend Success ";
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
void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    // QDataStream是Qt提供的用于二进制数据序列化的类,简单的工作原理:
    // 写入过程:类型T -> QDataStream -> QByteArray
    // 读取过程:QByteArray -> QDataStream -> 类型T
    uint16_t id = reqId;

    // 计算长度（使用网络字节序转换）
    //quint16 len = static_cast<quint16>(dataBytes.size());
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