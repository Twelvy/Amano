#pragma once

#include "Config.h"

namespace Amano {

class Device;

class Queue
{
public:
	Queue(Device* device, uint32_t familyIndex);
	~Queue();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
	Device* m_device;
	VkQueue m_queue;
	VkCommandPool m_commandPool;
};

}
