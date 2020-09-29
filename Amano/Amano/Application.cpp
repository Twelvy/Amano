#include "Application.h"

#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/FramebufferBuilder.h"
#include "Builder/PipelineBuilder.h"
#include "Builder/PipelineLayoutBuilder.h"
#include "Builder/RenderPassBuilder.h"
#include "Image.h"

#include <iostream>
#include <vector>

// temporary
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

namespace {

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Amano::Application*>(glfwGetWindowUserPointer(window));
	app->notifyFramebufferResized(width, height);
}

}

namespace Amano {

Application::Application()
	: m_window{ nullptr }
	, m_device{ nullptr }
	, m_framebufferResized{ false }
	, m_width{ 0 }
	, m_height{ 0 }
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

	/////////////////////////////////////////////
	// from here, this is a test application
	/////////////////////////////////////////////

	// TODO: check the supported formats for depth
	const VkFormat depthFormat = m_device->findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_SRGB
	const VkFormat normalFormat = VK_FORMAT_R8G8B8A8_UNORM;

	// create the depth image/buffer
	Image depthImage(m_device);
	depthImage.create(
		m_width,
		m_height,
		1,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkImageView depthImageView = depthImage.createView(VK_IMAGE_ASPECT_DEPTH_BIT);
	depthImage.transitionLayout(*m_device->getQueue(QueueType::eGraphics), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// create the color images/buffers
	Image colorImage(m_device);
	colorImage.create(
		m_width,
		m_height,
		1,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkImageView colorImageView = colorImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);

	Image normalImage(m_device);
	normalImage.create(
		m_width,
		m_height,
		1,
		normalFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkImageView normalImageView = normalImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);

	// create the render pass
	RenderPassBuilder renderPassBuilder;
	renderPassBuilder
		.addColorAttachment(colorFormat) // attachment 0 for color
		.addColorAttachment(normalFormat) // attachment 1 for normal
		.addDepthAttachment(depthFormat) // attachment 2 for depth
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, { 0, 1 }, 2) // subpass 0
		.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0);
	VkRenderPass renderPass = renderPassBuilder.build(*m_device);
	
	// create the framebuffer
	FramebufferBuilder framebufferBuilder;
	framebufferBuilder
		.addAttachment(colorImageView)
		.addAttachment(normalImageView)
		.addAttachment(depthImageView);
	VkFramebuffer framebuffer = framebufferBuilder.build(*m_device, renderPass, m_width, m_height);

	// create layout for the next pipeline
	DescriptorSetLayoutBuilder descriptorSetLayoutbuilder;
	descriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayoutbuilder.build(*m_device);
	
	// create pipeline layout
	PipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayoutBuilder.addDescriptorSetLayout(descriptorSetLayout);
	VkPipelineLayout pipelineLayout = pipelineLayoutBuilder.build(*m_device);

	// create graphics pipeline
	PipelineBuilder pipelineBuilder(*m_device);
	pipelineBuilder
		.addShader("../../compiled_shaders/gbufferv.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.addShader("../../compiled_shaders/gbufferf.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.setViewport(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f)
		.setScissor(0, 0, m_width, m_height)
		.setRasterizer(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	VkPipeline pipeline = pipelineBuilder.build(pipelineLayout, renderPass, 0, 2, true);

	return true;
}

void Application::run() {
	
	//initVulkan();
	mainLoop();
	//cleanup();
}

void Application::notifyFramebufferResized(int width, int height) {
	m_framebufferResized = true;
	m_width = static_cast<uint32_t>(width);
	m_height = static_cast<uint32_t>(height);
}

void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_width = static_cast<uint32_t>(WIDTH);
	m_height = static_cast<uint32_t>(HEIGHT);
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
