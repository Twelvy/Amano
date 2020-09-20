#pragma once

#include "Config.h"

namespace Amano {

class Device {

public:
	Device();
	~Device();

private:

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;

	//QueueFamilyIndices m_queueFamilyIndices;
};

}
