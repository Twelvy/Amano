#pragma once

#include "Config.h"
#include "Queue.h"

#include <vector>

namespace Amano {

enum class QueueType : uint32_t {
	eGraphics = 0,
	ePresent = 1,
	// eCompute
	// eTransfer
	eCount = 2
};

class Device {

public:
	Device();
	~Device();

	bool init(GLFWwindow* window);

	VkDevice handle() { return m_device; };

	void waitIdle();

	Queue* getQueue(QueueType type) { return m_queues[static_cast<uint32_t>(type)]; }
	VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool doesSuportBlitting(VkFormat format);

	bool createBufferAndMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void destroyBuffer(VkBuffer buffer);
	VkDeviceMemory allocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);
	void freeDeviceMemory(VkDeviceMemory deviceMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, QueueType queueType);

private:
	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface(GLFWwindow* window);
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool createQueues();
	bool createSwapChain(GLFWwindow* window);
	bool createDescriptorPool();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	VkDescriptorPool m_descriptorPool;

	Queue* m_queues[static_cast<uint32_t>(QueueType::eCount)];
};

}
