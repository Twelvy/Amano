#pragma once

#include "Config.h"

#include <vector>

namespace Amano {

class Device {

public:
	Device();
	~Device();

	bool init(GLFWwindow* window);

	VkDevice handle() { return m_device; };

private:
	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface(GLFWwindow* window);
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool createSwapChain(GLFWwindow* window);
	bool createCommandPool();

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

	VkQueue m_graphicsQueue;
	//VkQueue m_computeQueue;
	//VkQueue m_transferQueue;
	VkQueue m_presentQueue;

	VkCommandPool m_graphicsQueueCommandPool;
};

}
