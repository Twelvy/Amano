#include "Application.h"

#include "Builder/DescriptorSetBuilder.h"
#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/FramebufferBuilder.h"
#include "Builder/PipelineBuilder.h"
#include "Builder/PipelineLayoutBuilder.h"
#include "Builder/RenderPassBuilder.h"
#include "Builder/SamplerBuilder.h"

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

struct PerFrameUniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

}

namespace Amano {

Application::Application()
	: m_window{ nullptr }
	, m_device{ nullptr }
	, m_framebufferResized{ false }
	, m_width{ 0 }
	, m_height{ 0 }
	// necessary information to dislay the model
	, m_depthImage{ nullptr }
	, m_depthImageView{ VK_NULL_HANDLE }
	, m_colorImage{ nullptr }
	, m_colorImageView{ VK_NULL_HANDLE }
	, m_normalImage{ nullptr }
	, m_normalImageView{ VK_NULL_HANDLE }
	, m_renderPass{ VK_NULL_HANDLE }
	, m_framebuffer{ VK_NULL_HANDLE }
	, m_descriptorSetLayout{ VK_NULL_HANDLE }
	, m_pipelineLayout{ VK_NULL_HANDLE }
	, m_pipeline{ VK_NULL_HANDLE }
	, m_uniformBuffer{ nullptr }
	, m_model{ nullptr }
	, m_modelTexture{ nullptr }
	, m_modelTextureView{ VK_NULL_HANDLE }
	, m_sampler{ VK_NULL_HANDLE }
	, m_descriptorSet{ VK_NULL_HANDLE }
{
}

Application::~Application() {
	// TODO: wrap as many of those members into classes that know how to delete the Vulkan objects
	vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSet);
	vkDestroyImageView(m_device->handle(), m_modelTextureView, nullptr);
	vkDestroySampler(m_device->handle(), m_sampler, nullptr);
	delete m_modelTexture;
	delete m_model;
	delete m_uniformBuffer;
	vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
	vkDestroyRenderPass(m_device->handle(), m_renderPass, nullptr);
	vkDestroyFramebuffer(m_device->handle(), m_framebuffer, nullptr);
	vkDestroyImageView(m_device->handle(), m_normalImageView, nullptr);
	delete m_normalImage;
	vkDestroyImageView(m_device->handle(), m_colorImageView, nullptr);
	delete m_colorImage;
	vkDestroyImageView(m_device->handle(), m_depthImageView, nullptr);
	delete m_depthImage;

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
	m_depthImage = new Image(m_device);
	m_depthImage->create(
		m_width,
		m_height,
		1,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_depthImageView = m_depthImage->createView(VK_IMAGE_ASPECT_DEPTH_BIT);
	m_depthImage->transitionLayout(*m_device->getQueue(QueueType::eGraphics), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// create the color images/buffers
	m_colorImage = new Image(m_device);
	m_colorImage->create(
		m_width,
		m_height,
		1,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_colorImageView = m_colorImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

	m_normalImage = new Image(m_device);
	m_normalImage->create(
		m_width,
		m_height,
		1,
		normalFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_normalImageView = m_normalImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

	// create the render pass
	RenderPassBuilder renderPassBuilder;
	renderPassBuilder
		.addColorAttachment(colorFormat) // attachment 0 for color
		.addColorAttachment(normalFormat) // attachment 1 for normal
		.addDepthAttachment(depthFormat) // attachment 2 for depth
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, { 0, 1 }, 2) // subpass 0
		.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0);
	m_renderPass = renderPassBuilder.build(*m_device);
	
	// create the framebuffer
	FramebufferBuilder framebufferBuilder;
	framebufferBuilder
		.addAttachment(m_colorImageView)
		.addAttachment(m_normalImageView)
		.addAttachment(m_depthImageView);
	m_framebuffer = framebufferBuilder.build(*m_device, m_renderPass, m_width, m_height);

	// create layout for the next pipeline
	DescriptorSetLayoutBuilder descriptorSetLayoutbuilder;
	descriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_descriptorSetLayout = descriptorSetLayoutbuilder.build(*m_device);
	
	// create pipeline layout
	PipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayoutBuilder.addDescriptorSetLayout(m_descriptorSetLayout);
	m_pipelineLayout = pipelineLayoutBuilder.build(*m_device);

	// create graphics pipeline
	PipelineBuilder pipelineBuilder(*m_device);
	pipelineBuilder
		.addShader("../../compiled_shaders/gbufferv.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.addShader("../../compiled_shaders/gbufferf.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.setViewport(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f)
		.setScissor(0, 0, m_width, m_height)
		.setRasterizer(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	m_pipeline = pipelineBuilder.build(m_pipelineLayout, m_renderPass, 0, 2, true);

	// create the uniform buffer of the shader we are using
	m_uniformBuffer = new UniformBuffer<PerFrameUniformBufferObject>(m_device);

	// load the model to display
	m_model = new Model(m_device);
	m_model->create("../../assets/models/viking_room.obj");
	m_modelTexture = new Image(m_device);
	m_modelTexture->create("../../assets/textures/viking_room.png", *m_device->getQueue(QueueType::eGraphics));
	m_modelTextureView = m_modelTexture->createView(VK_IMAGE_ASPECT_COLOR_BIT);

	SamplerBuilder samplerBuilder;
	samplerBuilder.setMaxLoad((float)m_modelTexture->getMipLevels());
	m_sampler = samplerBuilder.build(*m_device);

	// update the descriptor set
	DescriptorSetBuilder descriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
	descriptorSetBuilder
		.addUniformBuffer(m_uniformBuffer->getBuffer(), sizeof(PerFrameUniformBufferObject), 0)
		.addImage(m_sampler, m_modelTextureView, 1);
	m_descriptorSet = descriptorSetBuilder.buildAndUpdate();

	return true;
}

void Application::run() {
	
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

	m_device->waitIdle();
}

}
