#pragma once
#include <boost/asio.hpp>
#include <vector>

#include "Singleton.h"

class IOServicePool : public Singleton<IOServicePool>
{
private:
	IOServicePool(std::size_t size = std::thread::hardware_concurrency());       // hardware_concurrency函数获取cpu的核数，根据cpu核数构造相同数量的线程

	using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	std::vector<boost::asio::io_context> _IOServices;                            // io_context池
	std::vector<std::unique_ptr<Work>> _works;                                   
	std::vector<std::thread> _threads;                                           // 线程池
	std::size_t _nextIOService;
public:
	friend class Singleton<IOServicePool>;                  // 声明友元类(因为单例基类需要访问IOServicePool的构造函数)
	~IOServicePool();
	IOServicePool(const IOServicePool&) = delete;
	IOServicePool& operator=(const IOServicePool&) = delete;

	boost::asio::io_context& GetIOService();                // 使用 round-robin 的方式返回一个 io_context
	void Stop();                                            // 终止IOServicePool
};

