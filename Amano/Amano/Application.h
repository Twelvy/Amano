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
	void notifyFramebufferResized(int width, int height);

private:
	void initWindow();
	void mainLoop();
	//void cleanup();

private:
	GLFWwindow* m_window;
	Device* m_device;
	bool m_framebufferResized;
	uint32_t m_width;
	uint32_t m_height;
};

}
