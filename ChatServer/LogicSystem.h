#pragma once
// 通过继承单例模板类实现逻辑系统 

#include "Singleton.h"
#include "const.h"
#include "CSession.h"
#include "data.h"

// 定义一个新类型FunCallBack，返回值为void， 第一个参数为指向Session的智能指针，第二个参数为消息id,第三个参数为消息内容
typedef std::function<void(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;                   // 声明友元类(因为基类中创建实例时使用了new开辟子类指针，需要访问子类的构造函数)

private:
	LogicSystem();
	void DealMsg();                                       // 处理函数，由工作线程调用
	void RegisterCallBack();                              // 注册回调函数
	void LoginHandler(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);

	std::queue<std::shared_ptr<LogicNode>> _msg_queue;    // 逻辑队列
	std::mutex _logic_mtx;                                // 保证逻辑队列线程安全性
	std::condition_variable _consume;                     // 条件变量，队列为空时将线程挂起，释放cpu资源
	std::thread _worker_thread;                           // 工作线程，从逻辑队列中取数据进行处理
	bool _b_stop;                                         // 标志位，接收来自上层或网络层的停服信号(可以不要)
	std::map<short, FunCallBack> _fun_callback;           // 将消息id与回调函数绑定起来
	std::unordered_map<int, std::shared_ptr<UserInfo>> _user;

public:
	~LogicSystem();
	void PostMsgToQueue(std::shared_ptr<LogicNode> msg);  // 将封装后的逻辑节点投递到逻辑队列中
};

