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

struct BufferAndMemory {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
};

class Device {

public:
	Device();
	~Device();

	bool init(GLFWwindow* window);

	VkDevice handle() { return m_device; };

	Queue* getQueue(QueueType type) { return m_queues[static_cast<uint32_t>(type)]; }
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool doesSuportBlitting(VkFormat format);

	BufferAndMemory createBufferAndMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void destroyBuffer(VkBuffer buffer);
	VkDeviceMemory allocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);
	void freeDeviceMemory(VkDeviceMemory deviceMemory);

private:
	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface(GLFWwindow* window);
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool createQueues();
	bool createSwapChain(GLFWwindow* window);

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

	Queue* m_queues[static_cast<uint32_t>(QueueType::eCount)];
};

}
