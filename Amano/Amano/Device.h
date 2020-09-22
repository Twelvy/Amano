#pragma once

#include "Config.h"

#include <vector>

namespace Amano {

class Device {

public:
	Device();
	~Device();

	bool init(VkInstance instance, GLFWwindow* window);

private:
	bool createSurface(VkInstance instance, GLFWwindow* window);
	bool pickPhysicalDevice(VkInstance instance);
	bool createLogicalDevice();
	bool createSwapChain(GLFWwindow* window);

private:
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
};

}
