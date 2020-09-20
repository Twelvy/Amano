#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Amano {

class Application {
public:
	Application();

	void run();
	void notifyFramebufferResized();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

private:
	GLFWwindow* m_window;
	bool m_framebufferResized;
};

}