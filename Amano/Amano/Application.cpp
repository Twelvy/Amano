#include "Application.h"

#include <iostream>
#include <vector>

// temporary
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

namespace {

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Amano::Application*>(glfwGetWindowUserPointer(window));
	app->notifyFramebufferResized();
}

}

namespace Amano {

Application::Application()
	: m_window{ nullptr }
	, m_device{ nullptr }
	, m_framebufferResized{ false }
{
}

Application::~Application() {
	delete m_device;
	m_device = nullptr;

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool Application::init() {
	initWindow();

	m_device = new Device();
	if (!m_device->init(m_window)) return false;

	return true;
}

void Application::run() {
	
	//initVulkan();
	mainLoop();
	//cleanup();
}

void Application::notifyFramebufferResized() {
	m_framebufferResized = true;
}

void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Amano Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		//drawFrame();
	}

	//vkDeviceWaitIdle(m_device);
}

}
