#pragma once
#include "Config.h"
#include "Device.h"

namespace Amano {

class Application {
public:
	Application();
	~Application();

	bool init();
	void run();
	void notifyFramebufferResized();

private:
	void initWindow();
	bool createInstance();
	bool setupDebugMessenger();

	//void initVulkan();
	void mainLoop();
	//void cleanup();

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	Device* m_device;
	bool m_framebufferResized;
};

}
