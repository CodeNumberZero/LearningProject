#pragma once
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <queue>
#include <iostream>

#include "MsgNode.h"
#include "const.h"

class Server;                                // 提前声明
class LogicSystem;

// 服务器用于通信的Session类
class Session : public std::enable_shared_from_this<Session>   // 奇异递归模板(介绍单例模式那一节讲过)
{
private:
	boost::asio::ip::tcp::socket _socket;                      // 处理客户端读写的socket
	char _data[MAX_LENGTH];                                    // 用来接收客户端传递的数据
	Server* _server;                                           // Session类所属的服务器
	std::string _session_id;                                   // Session类的uid
	bool _b_close;
	std::queue<std::shared_ptr<SendNode>> _send_queue;         // 消息队列，保证上一次消息发送完再发送下一条消息
	std::mutex _send_mtx;                                      // 保证发送队列安全性
	std::shared_ptr<ReceNode> _rece_msg_node;                  // 用于存储接受的消息体信息
	std::shared_ptr<MsgNode> _rece_head_node;                  // 用来存储接收的头部信息
	bool _b_head_parse;                                        // 表示是否解析完头部信息
	int _user_uid;											   // 当前Session关联的用户id
	std::atomic<time_t> _last_heartbeat;                       // 上一次心跳的时间戳(上次接受数据的时间)
	std::mutex _session_mtx;                                   // Session锁
	
	// 将Session的智能指针作为函数参数，避免回调函数在调用前Session就被释放掉了，从而延长Session的生命周期
	// 原理：利用智能指针被复制或使用引用计数加一的原理保证内存不被回收。bind操作可以将值绑定在一个函数对象上生成新的函数对象，如果将智能指针作为参数绑定给函数对象，那么智能指针就以值的方式被新函数对象使用，那么智能指针的生命周期将和新生成的函数对象一致，从而达到延长生命周期的效果
	void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<Session> _self_shared); // 读回调函数
	void HandleWrite(const boost::system::error_code& ec, std::shared_ptr<Session> shared_self);      // 写回调函数。在介绍异步api时提过，写一般一次写完，读一般多次读。因此写回调函数没有bytes_transferred参数，读回调函数则有
	void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler);
	void asyncReadLen(std::size_t read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)> handler);
public:
	Session(boost::asio::io_context& ioc, Server* server);     // 构造函数，根据给定的上下文创建一个socket用于通信
	~Session();

	// 注意:_socket和_uuid是私有变量，函数返回的是引用，为了防止外部修改最好加const修饰
	boost::asio::ip::tcp::socket& GetSocket();                 // 获取socket的接口
	const std::string& GetSessionId() const;                   // 获取当前Session的uuid
	void SetUserId(int uid);                                   // 设置当前Session关联的用户id
	int GetUserId();										   // 获取当前Session关联的用户id
	void Start();                                              // 启动函数，启动后服务端开始监听和接受客户端的消息
	void Send(char* msg, short max_length, short msg_id);      // 封装的发送接口
	void Send(std::string msg, short msg_id);
	void Close();                                              // 关闭socket连接
	std::shared_ptr<Session> SharedSelf();                     // 获取指向对象自己的智能指针
	void AsyncReadHead(int total_len);
	void AsyncReadBody(int total_len);
	void NotifyOffline(int uid);
	bool IsHeartbeatExpired(std::time_t& now);                 // 判断心跳是否过期
	void UpdateHeartbeat();                                    // 更新心跳
	//void DealExceptionSession();                               // 处理异常连接
};


// 逻辑节点(会话层触发回调函数后将数据封装为逻辑节点再投递到逻辑队列中等待逻辑层处理)
class LogicNode
{
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<Session> session, std::shared_ptr<ReceNode> rece_node); // 包含了会话类的智能指针，主要是为了实现伪闭包，防止session被释放
private:
	std::shared_ptr<Session> _session;                        // session的智能指针
	std::shared_ptr<ReceNode> _rece_node;                     // 接收节点的智能指针
};

