#pragma once

namespace Amano {

class Device;

class Queue
{
public:
	Queue(Device* device, uint32_t familyIndex);
	~Queue();

	VkQueue handle() { return m_queue; }

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkCommandBuffer beginCommands();
	bool endCommands(VkCommandBuffer commandBuffer);

	void freeCommandBuffer(VkCommandBuffer commandBuffer);

	bool submit(VkSubmitInfo* submitInfo, VkFence fence);

private:
	Device* m_device;
	VkQueue m_queue;
	VkCommandPool m_commandPool;
};

}
