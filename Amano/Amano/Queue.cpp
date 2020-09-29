#include "Queue.h"
#include "Device.h"

#include <iostream>

namespace Amano {

Queue::Queue(Device* device, uint32_t familyIndex)
	: m_device{ device }
	, m_queue{ VK_NULL_HANDLE }
	, m_commandPool{ VK_NULL_HANDLE }
{
	vkGetDeviceQueue(m_device->handle(), familyIndex, 0, &m_queue);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = familyIndex;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(m_device->handle(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
		std::cerr << "failed to create queue command pool!" << std::endl;
	}

	// NOTE: we assume ths will always succeed. This isn't true
}

Queue::~Queue() {
	vkDestroyCommandPool(m_device->handle(), m_commandPool, nullptr);
}

VkCommandBuffer Queue::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device->handle(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Queue::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_queue);

	vkFreeCommandBuffers(m_device->handle(), m_commandPool, 1, &commandBuffer);
}

}