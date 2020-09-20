#pragma once
#include "Config.h"

namespace Amano {

class Application {
public:
	Application();
	~Application();

	void run();
	void notifyFramebufferResized();

private:
	void initWindow();
	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface();

	//void initVulkan();
	void mainLoop();
	//void cleanup();

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	bool m_framebufferResized;
};

}
