#include "IOServicePool.h"

IOServicePool::IOServicePool(std::size_t size) : _IOServices(size), _works(size), _nextIOService(0){
	for (std::size_t i = 0; i < size; ++i) {
		_works[i] = std::make_unique<Work>(boost::asio::make_work_guard(_IOServices[i]));  // 初始化
		//_morks[i]＝std::make_unique<Work>(_ioServers[i].get_executor)
	}

	// 遍历多个io_service,创建多个线程，每个线程内部启动io_service
	for (std::size_t i = 0; i < _IOServices.size(); ++i) {
		_threads.emplace_back([this, i]() {  // lambda表达式捕获this本质是一个引用捕获，不需要再使用一个&来修饰，想要值捕获this只能[*this]
			_IOServices[i].run();
			});  
	}
	// 这里提个小建议，很多大佬都不建议在构造函数中跨线程的传递this指针，如果构造函数抛出异常提前退出主线程导致该对象被析构或者其他的行为，那么多线程访问该对象很可能会出问题，此处建议二段式构造
}

IOServicePool::~IOServicePool()
{
	Stop();                                                         // 自己回收自己的资源
	std::cout << "IOServicePool destruct!" << std::endl;
}

boost::asio::io_context& IOServicePool::GetIOService()
{
	auto& service = _IOServices[_nextIOService++];
	if (_nextIOService == _IOServices.size()) {
		_nextIOService = 0;
	}

	return service;
}

void IOServicePool::Stop()
{
	//_works.clear();          // 如果只是想让 run() 自然退出，可以重置 _works
	// 因为仅仅执行work.stop()并不能让io_context的run()函数退出，还需要确保没有未完成的任务在io_context中排队等待执行
	// 当io_context已经绑定了读或写的监听事件后，还需要手动stop该服务
	for (auto& work : _works) {
		work->get_executor().context().stop(); // 让io_context停止运行
		work.reset();          // 让unique指针置空并释放，那么work的析构函数就会被调用
	}
	for (auto& t : _threads) {
		t.join();
	}
	std::cout << "IOServicePool Stop!" << std::endl;
}

