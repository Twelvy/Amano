#include "Application.h"

#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/RenderPassBuilder.h"

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

	{
		RenderPassBuilder builder;
		// TODO: check the supported formats for depth
		builder
			.addColorAttachment(VK_FORMAT_R8G8B8A8_UNORM) // attachment 0 for color
			.addDepthAttachment(VK_FORMAT_D24_UNORM_S8_UINT) // attachment 1 for depth
			.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, { 0 }, 1) // subpass 0
			.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0);
		VkRenderPass pass = builder.build(*m_device);
	}
	{
		DescriptorSetLayoutBuilder builder;
		builder
			.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		VkDescriptorSetLayout descriptorSetLayout = builder.build(*m_device);
	}

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
