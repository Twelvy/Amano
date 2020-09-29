#pragma once

#include "Config.h"
#include "Queue.h"

#include <vector>

namespace Amano {

class Device {
public:
	enum class QueueType : uint32_t {
		eGraphics = 0,
		ePresent = 1,
		// eCompute
		// eTransfer
		eCount = 2
	};

public:
	Device();
	~Device();

	bool init(GLFWwindow* window);

	VkDevice handle() { return m_device; };

	Queue* GetQueue(QueueType type) { return m_queues[static_cast<uint32_t>(type)]; }

private:
	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface(GLFWwindow* window);
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool createQueues();
	bool createSwapChain(GLFWwindow* window);

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
