#pragma once
#include "const.h"

// SectionInfo结构体用于管理key和value
struct SectionInfo {
	SectionInfo(){}                                                            // 默认构造函数：因为std::map有自己的默认构造函数,会自动初始化_section_datas为空map
	~SectionInfo();
	SectionInfo(const SectionInfo& src);                                       // 拷贝构造
	SectionInfo& operator=(const SectionInfo& src);                            // 重载=运算符
	std::string operator[](const std::string& key);                            // 重载[]运算符
	std::string GetValue(const std::string& key);

	std::map<std::string, std::string> _section_datas;
};

// ConfigMgr类用于读取和管理配置(使用单例模式)
class ConfigMgr
{
private:
	ConfigMgr();                                                             
	ConfigMgr(const ConfigMgr& src) = delete;
	ConfigMgr& operator=(const ConfigMgr& src) = delete;

	std::map<std::string, SectionInfo> _config_map;                          // 存储section和key-value对的map  

public:
	~ConfigMgr();
	SectionInfo operator[](const std::string& section);                      // 重载[]运算符以通过section名称访问配置
	static ConfigMgr& GetInstance();                                         // 获取单例实例;C++11之后使用静态局部变量实现单例的方式(属于懒汉单例)
	std::string GetValue(const std::string& section, const std::string& key);
};

