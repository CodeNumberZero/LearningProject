#pragma once
#include <sw/redis++/redis++.h>
#include <string>
#include "const.h"

class DistributeLock
{
private:
	DistributeLock() = default;
	DistributeLock(const DistributeLock&) = delete;
	DistributeLock& operator=(const DistributeLock&) = delete;

public:
	~DistributeLock();
	static DistributeLock& GetInstance();
	std::string acquireLock(std::shared_ptr<sw::redis::Redis>& redis, const std::string& lockName, int lockTimeout, int acquireTimeout);
	bool releaseLock(std::shared_ptr<sw::redis::Redis>& redis, const std::string& lockName, const std::string& identifier);
};

