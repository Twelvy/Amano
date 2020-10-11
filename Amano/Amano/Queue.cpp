#include "Queue.h"
#include "Device.h"

#include <iostream>

namespace Amano {

Queue::Queue(Device* device, uint32_t familyIndex)
	: m_device{ device }
	, m_familyIndex{ familyIndex }
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

	submit(&submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_queue);

	freeCommandBuffer(commandBuffer);
}

VkCommandBuffer Queue::beginCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(m_device->handle(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
		std::cerr << "failed to allocate command buffers!" << std::endl;
		return VK_NULL_HANDLE;
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		std::cerr << "failed to begin recording command buffer!" << std::endl;
		freeCommandBuffer(commandBuffer);
		return VK_NULL_HANDLE;
	}

	return commandBuffer;
}

bool Queue::endCommands(VkCommandBuffer commandBuffer) {
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		std::cerr << "failed to record command buffer!" << std::endl;
		return false;
	}

	return true;
}

void Queue::freeCommandBuffer(VkCommandBuffer commandBuffer) {
	vkFreeCommandBuffers(m_device->handle(), m_commandPool, 1, &commandBuffer);
}

bool Queue::submit(VkSubmitInfo* submitInfo, VkFence fence) {
	if (vkQueueSubmit(m_queue, 1, submitInfo, fence) != VK_SUCCESS) {
		std::cerr << "failed to submit draw command buffer!" << std::endl;
		return false;
	}
	return true;
}

}