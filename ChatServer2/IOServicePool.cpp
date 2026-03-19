#include "IOServicePool.h"

IOServicePool::IOServicePool(std::size_t size) : _IOServices(size), _works(size), _nextIOService(0){
	for (std::size_t i = 0; i < size; ++i) {
		_works[i] = std::make_unique<Work>(boost::asio::make_work_guard(_IOServices[i]));  // 初始化
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
	Stop();
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
	for (auto& work : _works) {
		//// 1. 先 reset 所有 work_guard
		//work.reset();

		//// 1. 先 reset work_guard 释放对 io_context 的持有
		//work.reset();
		//// 2. 然后停止 io_context
		//auto& ioc = work->get_executor().context();  // 获取 io_context
		//ioc.stop();

		work->get_executor().context().stop();
		work.reset();          // 让unique指针置空并释放，那么work的析构函数就会被调用
	}

	//// 2. 停止所有 io_context
	//for (auto& io_context : _IOServices) {
	//	if (!io_context.stopped()) {
	//		io_context.stop();
	//	}
	//}

	for (auto& t : _threads) {
		t.join();
	}
	std::cout << "IOServicePool Stop!" << std::endl;
}

