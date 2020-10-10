#pragma once

#include "glfw.h"
#include "Extensions.h"
#include "Queue.h"

#include <vector>

namespace Amano {

enum class QueueType : uint32_t {
	eGraphics = 0,
	eCompute = 1,
	ePresent = 2,
	// eTransfer
	eCount = 3
};

class Device {

public:
	Device();
	~Device();

	bool init(GLFWwindow* window);

	VkInstance instance() { return m_instance; }
	VkPhysicalDevice physicalDevice() { return m_physicalDevice; }
	VkDevice handle() { return m_device; };
	const Extensions& getExtensions() const { return m_extensions; }
	VkPhysicalDeviceRayTracingPropertiesNV getRaytracingPhysicalProperties();

	void recreateSwapChain(GLFWwindow* window);

	Queue* getQueue(QueueType type) { return m_queues[static_cast<uint32_t>(type)]; }
	VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }
	VkFormat getSwapChainFormat() const { return m_swapChainImageFormat; }
	std::vector<VkImage>& getSwapChainImages() { return m_swapChainImages; }
	std::vector<VkImageView>& getSwapChainImageViews() { return m_swapChainImageViews; }

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool doesSuportBlitting(VkFormat format);

	void waitIdle();
	VkResult acquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex);
	VkResult present(VkSemaphore waitSemaphore, uint32_t imageIndex);
	void wait();

	bool createBufferAndMemory(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void destroyBuffer(VkBuffer buffer);
	VkDeviceMemory allocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);
	void freeDeviceMemory(VkDeviceMemory deviceMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, QueueType queueType);

	// those two methods are basically used for IMGUI only
	VkDescriptorPool createDetachedDescriptorPool();
	void releaseDescriptorPool(VkDescriptorPool descriptorPool);

private:
	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface(GLFWwindow* window);
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool createQueues();
	bool createSwapChain(GLFWwindow* window);
	bool createDescriptorPool();

	void destroySwapChain();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	VkDescriptorPool m_descriptorPool;

	Queue* m_queues[static_cast<uint32_t>(QueueType::eCount)];

	Extensions m_extensions;
};

}
