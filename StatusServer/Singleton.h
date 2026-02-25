#pragma once
#include "const.h"

/*
	模板类的声明和实现要写在一起(模板类的实现基本都在头文件里实现，在源文件里面实现会在编译时会识别不到)
*/

// 单例基类
template <typename T>
class Singleton {
protected:
	/*
		1、默认构造设为保护，禁用拷贝构造和拷贝复制
		2、当这三个函数设为protected时，子类能够调用，而外界无法调用(因为在构造子类时按照顺序需要先构造基类再构造子类，因此要确保子类能调用基类的构造，如果设成private子类就无法调用了)
		3、C++模板的语法规定：在类模板的定义体内，当引用该类模板自身时,可以省略模板参数列表,直接使用类模板的名称,即在类中Singleton&和Singleton<T>&是等价的。(类内写不写T都一样，类外必须要写，不然不知道是哪个类)
	*/
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>&) = delete;

	static std::shared_ptr<T> _instance;                    // 使用静态成员变量，确保只有一个实例
	/*
		另一种声明和定义静态变量的方法。
		C++17及以上,使用inline修饰静态成员变量,可以直接在类内完成定义(初始化),无需类外单独写定义语句。如下:
		inline static std::shared_ptr<T> _instance = nullptr;
	*/
public:
	static std::shared_ptr<T> GetInstance() {
		static std::once_flag flag;                         // 标志位，用于标记std::call_once调用的目标函数是否已执行  
		std::call_once(flag, []() {                         // std::call_once 是C++11引入的线程安全工具，核心作用是保证某个函数/操作在多线程环境下"仅被执行一次"(即使多个线程同时调用)
			//_instance = std::make_shared<T>();            // 不能使用这种方式构造。原因:make_shared需要调用构造函数,而这里的托管对象是单例,构造设置为私有了,make_shared无权限调用
			_instance = std::shared_ptr<T>(new T);          // 能用new进行构造是因为：new是在类的成员函数内使用的，而类的成员函数本身就有权限访问私有构造函数
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

	/*
		1、在这里将析构设为公有的原因：单例基类中有个成员变量_instance，在析构时会回收这个变量，而这个变量又是个智能指针，指针指向子类对象，
		   所以回收变量就是析构智能指针，析构智能指针又要析构其指向的子类对象，而析构子类对象要调用子类的析构函数，如果子类的析构设为私有就无法调用了
		2、析构也可以设置为私有。但是如果设为私有，子类析构时就无法调用，这种情况就需要使用辅助类作为删除器来进行析构
	*/
	~Singleton() {                                          
		std::cout << "This is Singleton destruct!" << std::endl;
	}
};

/*
	静态成员变量的初始化，必须放在.h文件中初始化，不能在.cpp文件中初始化(放在头文件的类模板定义之后,避免编译链接错误)
*/
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
