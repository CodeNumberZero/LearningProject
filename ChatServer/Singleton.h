#pragma once
// 实现单例基类
// 模版在实例化时，编译器需要看到模版的定义，所以模板类最好放在.h中

#include <memory>
#include <iostream>
#include <mutex>

template <typename T>
class Singleton {
protected:
	// 默认构造设为保护，禁用拷贝构造和拷贝复制
	// 当这三个函数设为protected时，子类能够调用，而外界无法调用(因为在构造子类时按照顺序需要先构造基类再构造子类，因此要确保子类能调用基类的构造，如果设成private子类就无法调用了)
	Singleton() = default;
	Singleton(const Singleton<T>& s) = delete;
	Singleton& operator=(const Singleton<T>& s) = delete;

	static std::shared_ptr<T> _instance;

public:
	~Singleton() {            // 析构也可以设置为私有。但是如果设为私有，子类无法调用，就需要使用辅助类作为删除器来进行析构
		std::cout << "Singleton desturct!" << std::endl;
	}

	static std::shared_ptr<T> GetInstance() {
		static std::once_flag flag;  // 标志位，用于标记std::call_once调用的目标函数是否已执行      
		std::call_once(flag, [&]() { // std::call_once 是C++11引入的线程安全工具，核心作用是保证某个函数/操作在多线程环境下"仅被执行一次"(即使多个线程同时调用)
			_instance = std::shared_ptr<T>(new T);
			//_instance = std::make_shared<T>();
			});
		return _instance;
	}

	/*
	另一种构造单例实例的方法。如下：
	static T& GetInstance() {
		static T instance;
		return instance;
	}
	原理：static变量生命周期随同程序，而在C++11之后，static局部变量的初始化是线程安全的(静态局部变量的初始化只会在控制首次进入包含它的作用域时发生，且之后不会重复初始化)
    */

	void PrintAddress() {
		std::cout << _instance.get() << std::endl;
	}
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr; //静态成员变量的初始化，必须放在.h文件中初始化，不能在.cpp文件中初始化


