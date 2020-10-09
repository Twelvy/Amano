#include "Application.h"

#include "Builder/DescriptorSetBuilder.h"
#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/FramebufferBuilder.h"
#include "Builder/PipelineBuilder.h"
#include "Builder/PipelineLayoutBuilder.h"
#include "Builder/RaytracingPipelineBuilder.h"
#include "Builder/RenderPassBuilder.h"
#include "Builder/SamplerBuilder.h"
#include "Builder/TransitionImageBarrierBuilder.h"

#include <imgui.h>

#include <chrono>
#include <iostream>
#include <vector>

// temporary
// Default window width and height
const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
const int MAX_FRAMES_IN_FLIGHT = 2;

namespace {

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Amano::Application*>(glfwGetWindowUserPointer(window));
	app->notifyFramebufferResized(width, height);
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	auto app = reinterpret_cast<Amano::Application*>(glfwGetWindowUserPointer(window));
	app->onScrollCallback(xoffset, yoffset);
}

}

namespace Amano {

Application::Application()
	: m_window{ nullptr }
	, m_framebufferResized{ false }
	, m_width{ 0 }
	, m_height{ 0 }
	, m_device{ nullptr }
	, m_inputSystem{ nullptr }
	, m_guiSystem{ nullptr }
	, m_debugOrbitCamera{ nullptr }
	, m_finalFramebuffers()
	// necessary information to display the model
	, m_depthImage{ nullptr }
	, m_GBuffer{}
	, m_renderPass{ VK_NULL_HANDLE }
	, m_framebuffer{ VK_NULL_HANDLE }
	, m_descriptorSetLayout{ VK_NULL_HANDLE }
	, m_pipelineLayout{ VK_NULL_HANDLE }
	, m_pipeline{ VK_NULL_HANDLE }
	, m_uniformBuffer{ nullptr }
	, m_lightUniformBuffer{ nullptr }
	, m_mesh{ nullptr }
	, m_modelTexture{ nullptr }
	, m_sampler{ VK_NULL_HANDLE }
	, m_descriptorSet{ VK_NULL_HANDLE }
	, m_imageAvailableSemaphore{ VK_NULL_HANDLE }
	, m_renderFinishedSemaphore{ VK_NULL_HANDLE }
	, m_raytracingFinishedSemaphore{ VK_NULL_HANDLE }
	, m_blitFinishedSemaphore{ VK_NULL_HANDLE }
	, m_inFlightFence{ VK_NULL_HANDLE }
	, m_blitFence{ VK_NULL_HANDLE }
	, m_renderCommandBuffer{ VK_NULL_HANDLE }
	, m_blitCommandBuffers()
	, m_raytracingImage{ nullptr }
	, m_raytracingUniformBuffer{ nullptr }
	, m_raytracingDescriptorSetLayout{ VK_NULL_HANDLE }
	, m_raytracingPipelineLayout{ VK_NULL_HANDLE }
	, m_raytracingPipeline{ VK_NULL_HANDLE }
	, m_raytracingDescriptorSet{ VK_NULL_HANDLE }
	, m_accelerationStructures{}
	, m_shaderBindingTables{}
	, m_raytracingCommandBuffer{ VK_NULL_HANDLE }
	// light information
	, m_lightPosition(1.0f, 1.0f, 1.0f)
{
}

Application::~Application() {
	// TODO: wrap as many of those members into classes that know how to delete the Vulkan objects
	cleanSizedependentObjects();
	m_accelerationStructures.clean(m_device);
	m_shaderBindingTables.clean(m_device);
	vkDestroyPipeline(m_device->handle(), m_raytracingPipeline, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_raytracingPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->handle(), m_raytracingDescriptorSetLayout, nullptr);
	delete m_raytracingUniformBuffer;

	vkDestroySemaphore(m_device->handle(), m_blitFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device->handle(), m_raytracingFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device->handle(), m_renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device->handle(), m_imageAvailableSemaphore, nullptr);
	vkDestroyFence(m_device->handle(), m_inFlightFence, nullptr);
	vkDestroyFence(m_device->handle(), m_raytracingFence, nullptr);
	vkDestroyFence(m_device->handle(), m_blitFence, nullptr);

	vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSet);
	vkDestroySampler(m_device->handle(), m_sampler, nullptr);
	delete m_modelTexture;
	delete m_mesh;
	delete m_uniformBuffer;
	delete m_lightUniformBuffer;
	vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
	vkDestroyRenderPass(m_device->handle(), m_renderPass, nullptr);

	delete m_inputSystem;
	delete m_debugOrbitCamera;
	delete m_guiSystem;

	delete m_device;
	m_device = nullptr;

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::run() {
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		if (m_inputSystem != nullptr)
			m_inputSystem->update(m_window);
		drawFrame();
	}

	m_device->waitIdle();
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
	// NOTE: the window cannot be resized for now
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
	glfwSetScrollCallback(m_window, scrollCallback);
}

Application::Formats Application::getFormats() {
	Formats formats;
	formats.depthFormat = m_device->findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	formats.colorFormat = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_SRGB
	formats.normalFormat = VK_FORMAT_R8G8B8A8_SNORM;
	formats.depthFormat2 = VK_FORMAT_R32_SFLOAT;
	return formats;
}

void Application::recreateSwapChain() {
	if (m_device != nullptr) {
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}
		m_width = static_cast<uint32_t>(width);
		m_height = static_cast<uint32_t>(height);

		vkDeviceWaitIdle(m_device->handle());

		cleanSizedependentObjects();
		createSizeDependentObjects();
	}
}

void Application::createSizeDependentObjects() {
	if (m_debugOrbitCamera != nullptr)
		m_debugOrbitCamera->setFrameSize(m_width, m_height);

	if (m_device != nullptr) {
		m_device->recreateSwapChain(m_window);

		/////////////////////////////////////////////
		// from here, this is a test application
		/////////////////////////////////////////////

		// create the final framebuffers
		m_finalFramebuffers.reserve(m_device->getSwapChainImageViews().size());
		for (auto swapchainImageView : m_device->getSwapChainImageViews())
		{
			FramebufferBuilder finalFramebufferBuilder;
			finalFramebufferBuilder
				.addAttachment(swapchainImageView);
			m_finalFramebuffers.push_back(finalFramebufferBuilder.build(*m_device, m_guiSystem->renderPass(), m_width, m_height));
		}

		Formats formats = getFormats();

		// create the depth image/buffer
		m_depthImage = new Image(m_device);
		m_depthImage->create(
			m_width,
			m_height,
			1,
			formats.depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_depthImage->createView(VK_IMAGE_ASPECT_DEPTH_BIT);
		m_depthImage->transitionLayout(*m_device->getQueue(QueueType::eGraphics), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// create the images for the GBuffer
		m_GBuffer.colorImage = new Image(m_device);
		m_GBuffer.colorImage->create(
			m_width,
			m_height,
			1,
			formats.colorFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_GBuffer.colorImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBuffer.normalImage = new Image(m_device);
		m_GBuffer.normalImage->create(
			m_width,
			m_height,
			1,
			formats.normalFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_GBuffer.normalImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBuffer.depthImage = new Image(m_device);
		m_GBuffer.depthImage->create(
			m_width,
			m_height,
			1,
			formats.depthFormat2,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_GBuffer.depthImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

		// create the framebuffer
		FramebufferBuilder framebufferBuilder;
		framebufferBuilder
			.addAttachment(m_GBuffer.colorImage->viewHandle())
			.addAttachment(m_GBuffer.normalImage->viewHandle())
			.addAttachment(m_GBuffer.depthImage->viewHandle())
			.addAttachment(m_depthImage->viewHandle());
		m_framebuffer = framebufferBuilder.build(*m_device, m_renderPass, m_width, m_height);

		// update the descriptor set
		DescriptorSetBuilder descriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
		descriptorSetBuilder
			.addUniformBuffer(m_uniformBuffer->getBuffer(), m_uniformBuffer->getSize(), 0)
			.addImage(m_sampler, m_modelTexture->viewHandle(), 1)
			.addUniformBuffer(m_lightUniformBuffer->getBuffer(), m_lightUniformBuffer->getSize(), 2);
		m_descriptorSet = descriptorSetBuilder.buildAndUpdate();

		// create raytracing image
		m_raytracingImage = new Image(m_device);
		m_raytracingImage->create(
			m_width,
			m_height,
			1,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_raytracingImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

		// update the descriptor set for raytracing
		DescriptorSetBuilder raytracingDescriptorSetBuilder(m_device, 2, m_raytracingDescriptorSetLayout);
		raytracingDescriptorSetBuilder
			.addAccelerationStructure(&m_accelerationStructures.top.handle, 0)
			.addStorageImage(m_raytracingImage->viewHandle(), 1)
			.addUniformBuffer(m_raytracingUniformBuffer->getBuffer(), m_raytracingUniformBuffer->getSize(), 2)
			.addStorageImage(m_GBuffer.depthImage->viewHandle(), 3)
			.addStorageImage(m_GBuffer.normalImage->viewHandle(), 4)
			.addStorageImage(m_GBuffer.colorImage->viewHandle(), 5)
			.addUniformBuffer(m_lightUniformBuffer->getBuffer(), m_lightUniformBuffer->getSize(), 6);
		m_raytracingDescriptorSet = raytracingDescriptorSetBuilder.buildAndUpdate();

		// record the commands that will be executed, rendering and blitting
		recordRenderCommands();
		recordBlitCommands();
		recordRaytracingCommands();
	}
}

void Application::cleanSizedependentObjects() {
	vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_raytracingDescriptorSet);
	m_raytracingDescriptorSet = VK_NULL_HANDLE;
	delete m_raytracingImage;
	m_raytracingImage = nullptr;

	m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(m_renderCommandBuffer);
	m_renderCommandBuffer = VK_NULL_HANDLE;
	for (auto& cmd : m_blitCommandBuffers)
		m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(cmd);
	m_blitCommandBuffers.clear();

	vkDestroyFramebuffer(m_device->handle(), m_framebuffer, nullptr);
	m_framebuffer = VK_NULL_HANDLE;

	delete m_GBuffer.depthImage;
	m_GBuffer.depthImage = nullptr;
	delete m_GBuffer.normalImage;
	m_GBuffer.normalImage = nullptr;
	delete m_GBuffer.colorImage;
	m_GBuffer.colorImage = nullptr;
	delete m_depthImage;
	m_depthImage = nullptr;

	for (auto finalFb : m_finalFramebuffers)
		vkDestroyFramebuffer(m_device->handle(), finalFb, nullptr);
	m_finalFramebuffers.clear();
}

bool Application::init() {
	initWindow();

	m_device = new Device();
	if (!m_device->init(m_window)) return false;

	m_guiSystem = new ImGuiSystem(m_device);
	if (!m_guiSystem->init()) return false;

	m_debugOrbitCamera = new DebugOrbitCamera();

	m_inputSystem = new InputSystem();
	m_inputSystem->registerReader(m_guiSystem);
	m_inputSystem->registerReader(m_debugOrbitCamera);

	/////////////////////////////////////////////
	// from here, this is a test application
	/////////////////////////////////////////////

	Formats formats = getFormats();

	// create the render pass
	RenderPassBuilder renderPassBuilder;
	renderPassBuilder
		.addColorAttachment(formats.colorFormat, VK_IMAGE_LAYOUT_GENERAL) // attachment 0 for color
		.addColorAttachment(formats.normalFormat, VK_IMAGE_LAYOUT_GENERAL) // attachment 1 for normal
		.addColorAttachment(formats.depthFormat2, VK_IMAGE_LAYOUT_GENERAL) // attachment 2 for depth
		.addDepthAttachment(formats.depthFormat) // attachment 3 for depth buffer
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, { 0, 1, 2 }, 3) // subpass 0
		.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0);
	m_renderPass = renderPassBuilder.build(*m_device);
	
	// create layout for the next pipeline
	DescriptorSetLayoutBuilder descriptorSetLayoutbuilder;
	descriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_descriptorSetLayout = descriptorSetLayoutbuilder.build(*m_device);
	
	// create pipeline layout
	PipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayoutBuilder.addDescriptorSetLayout(m_descriptorSetLayout);
	m_pipelineLayout = pipelineLayoutBuilder.build(*m_device);

	// create graphics pipeline
	PipelineBuilder pipelineBuilder(m_device);
	pipelineBuilder
		.addShader("compiled_shaders/gbufferv.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.addShader("compiled_shaders/gbufferf.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		//.setViewport(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f)
		//.setScissor(0, 0, m_width, m_height)
		.setRasterizer(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	m_pipeline = pipelineBuilder.build(m_pipelineLayout, m_renderPass, 0, 3, true);

	// create the uniform buffer of the shader we are using
	m_uniformBuffer = new UniformBuffer<PerFrameUniformBufferObject>(m_device);
	m_lightUniformBuffer = new UniformBuffer<LightInformation>(m_device);

	// load the model to display
	m_mesh = new Mesh(m_device);
	m_mesh->create("assets/models/viking_room.obj");

	// load the texture of the model
	m_modelTexture = new Image(m_device);
	m_modelTexture->create("assets/textures/viking_room.png", *m_device->getQueue(QueueType::eGraphics));
	m_modelTexture->createView(VK_IMAGE_ASPECT_COLOR_BIT);

	// create a sampler for the texture
	SamplerBuilder samplerBuilder;
	samplerBuilder.setMaxLoad((float)m_modelTexture->getMipLevels());
	m_sampler = samplerBuilder.build(*m_device);

	// uniform buffer
	m_raytracingUniformBuffer = new UniformBuffer<RayParams>(m_device);

	// create layout for the raytracing pipeline
	DescriptorSetLayoutBuilder raytracingDescriptorSetLayoutbuilder;
	raytracingDescriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, VK_SHADER_STAGE_RAYGEN_BIT_NV)  // acceleration structure
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV)              // output image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV)             // ray parameters
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV)              // depth image
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV)              // normal image
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV)              // color image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);            // light information
	m_raytracingDescriptorSetLayout = raytracingDescriptorSetLayoutbuilder.build(*m_device);

	// create raytracing pipeline layout
	PipelineLayoutBuilder raytracingPipelineLayoutBuilder;
	raytracingPipelineLayoutBuilder.addDescriptorSetLayout(m_raytracingDescriptorSetLayout);
	m_raytracingPipelineLayout = raytracingPipelineLayoutBuilder.build(*m_device);

	// load the shaders and create the pipeline
	RaytracingPipelineBuilder raytracingPipelineBuilder(m_device);
	raytracingPipelineBuilder
		.addShader("compiled_shaders/shadow_raygen.spv", VK_SHADER_STAGE_RAYGEN_BIT_NV)
		.addShader("compiled_shaders/shadow_miss.spv", VK_SHADER_STAGE_MISS_BIT_NV)
		.addShader("compiled_shaders/shadow_closesthit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	m_raytracingPipeline = raytracingPipelineBuilder.build(m_raytracingPipelineLayout, 1);

	// create the shader binding table associated to this pipeline
	ShaderBindingTableBuilder sbtBuilder(m_device, m_raytracingPipeline);
	sbtBuilder
		.addShader(ShaderBindingTableBuilder::Stage::eRayGen, 0)
		.addShader(ShaderBindingTableBuilder::Stage::eMiss, 1)
		.addShader(ShaderBindingTableBuilder::Stage::eClosestHit, 2);
	m_shaderBindingTables = sbtBuilder.build();

	// build the acceleration structure for 1 model
	// we should extend it to more models
	RaytracingAccelerationStructureBuilder accelerationStructureBuilder(m_device, m_raytracingPipeline);
	accelerationStructureBuilder.addGeometry(*m_mesh);
	m_accelerationStructures = accelerationStructureBuilder.build();

	// create the sync objects
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_raytracingFinishedSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_blitFinishedSemaphore) != VK_SUCCESS) {

		std::cerr << "failed to create semaphores for a frame!" << std::endl;
		return false;
	}
	
	if (vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_blitFence) != VK_SUCCESS ||
		vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_raytracingFence) != VK_SUCCESS ||
		vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {

		std::cerr << "failed to create fences for a frame!" << std::endl;
		return false;
	}

	createSizeDependentObjects();
	
	return true;
}

void Application::onScrollCallback(double xscroll, double yscroll) {
	if (m_inputSystem != nullptr)
		m_inputSystem->updateScroll(xscroll, yscroll);
}

void Application::drawFrame() {
	// wait for the previous frame to finish
	// we could allow multiple frames at the same time in the future
	vkWaitForFences(m_device->handle(), 1, &m_blitFence, VK_TRUE, UINT64_MAX);
	vkWaitForFences(m_device->handle(), 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_device->handle(), 1, &m_blitFence);
	vkResetFences(m_device->handle(), 1, &m_raytracingFence);
	vkResetFences(m_device->handle(), 1, &m_inFlightFence);

	if (m_framebufferResized) {
		m_framebufferResized = false;
		recreateSwapChain();
	}

	// get the next image in the swapchain
	uint32_t imageIndex;
	auto result = m_device->acquireNextImage(m_imageAvailableSemaphore, imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		std::cerr << "failed to acquire swap chain image!" << std::endl;
	}

	// now that we know that the previous frame is finished, we can update the buffers
	updateUniformBuffer();

	// submit rendering
	// 1. wait for the image to be available
	// 2. signal the render finished semaphore
	// 3. notify the raytracing fence
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_renderCommandBuffer;
	
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	if (!pQueue->submit(&submitInfo, m_raytracingFence))
		return;

	// submit raytracing
	// 1. wait for the render finished semaphore
	// 2. signal the raytracing finished semaphore
	// 3. notify the blit fence
	VkSubmitInfo raytracingSubmitInfo{};
	raytracingSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore raytracingWaitSemaphores[] = { m_renderFinishedSemaphore };
	VkPipelineStageFlags raytracingWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	raytracingSubmitInfo.waitSemaphoreCount = 1;
	raytracingSubmitInfo.pWaitSemaphores = raytracingWaitSemaphores;
	raytracingSubmitInfo.pWaitDstStageMask = raytracingWaitStages;

	raytracingSubmitInfo.commandBufferCount = 1;
	raytracingSubmitInfo.pCommandBuffers = &m_raytracingCommandBuffer;

	VkSemaphore raytracingSignalSemaphores[] = { m_raytracingFinishedSemaphore };
	raytracingSubmitInfo.signalSemaphoreCount = 1;
	raytracingSubmitInfo.pSignalSemaphores = raytracingSignalSemaphores;

	if (!pQueue->submit(&raytracingSubmitInfo, m_blitFence))
		return;

	// submit blit
	// 1. wait for the raytracing finished semaphore
	// 2. signal the blit finished semaphore
	// 3. notify the in-flight fence
	VkSubmitInfo blitSubmitInfo{};
	blitSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore blitWaitSemaphores[] = { m_raytracingFinishedSemaphore };
	VkPipelineStageFlags blitWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	blitSubmitInfo.waitSemaphoreCount = 1;
	blitSubmitInfo.pWaitSemaphores = blitWaitSemaphores;
	blitSubmitInfo.pWaitDstStageMask = blitWaitStages;
	blitSubmitInfo.commandBufferCount = 1;
	blitSubmitInfo.pCommandBuffers = &m_blitCommandBuffers[imageIndex];

	VkSemaphore blitSignalSemaphores[] = { m_blitFinishedSemaphore };
	blitSubmitInfo.signalSemaphoreCount = 1;
	blitSubmitInfo.pSignalSemaphores = blitSignalSemaphores;

	if (!pQueue->submit(&blitSubmitInfo, m_inFlightFence))
		return;

	// udpate UI
	drawUI(imageIndex);
	
	// wait for the rendering to finish
	result = m_device->present(m_blitFinishedSemaphore, imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
		m_framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		std::cerr << "failed to present swap chain image!" << std::endl;
		return;
	}

	m_device->wait();
}

void Application::drawUI(uint32_t imageIndex) {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)m_width, (float)m_height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	m_guiSystem->startFrame();

	ImGui::Begin("Light information");
	ImGui::DragFloat3("position", &m_lightPosition[0], 0.01f, 1.0f, 1.0f);
	ImGui::End();

	m_guiSystem->endFrame(m_finalFramebuffers[imageIndex], m_width, m_height);
}

void Application::updateUniformBuffer() {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	//glm::vec3 origin = glm::vec3(2.8f * cosf(time), 2.8f * sinf(time), 2.0f);

	//float angleRadians = glm::radians(m_cameraAngle);
	//glm::vec3 origin = glm::vec3(2.8f * cosf(angleRadians), 2.8f * sinf(angleRadians), 2.0f);
	glm::vec3 origin = m_debugOrbitCamera->getCameraPosition();

	// update the gbuffer shader uniform
	PerFrameUniformBufferObject ubo{};
	//ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.model = glm::mat4(1.0f);
	ubo.view = glm::lookAt(origin, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_width / (float)m_height, 0.1f, 10.0f);

	// glm uses the opengl convention, so we need to flip the Y axis of the projection
	// TODO: fix this by implementing our own projection method
	ubo.proj[1][1] *= -1;

	m_uniformBuffer->update(ubo);

	// update the raytracing shader uniform
	RayParams rayUbo{};
	rayUbo.viewInverse = glm::inverse(ubo.view);
	rayUbo.projInverse = glm::inverse(ubo.proj);
	rayUbo.rayOrigin = origin;

	m_raytracingUniformBuffer->update(rayUbo);

	// update the light position
	LightInformation lightUbo;
	lightUbo.lightPosition = m_lightPosition;
	m_lightUniformBuffer->update(lightUbo);
}

void Application::recordRenderCommands() {
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_renderCommandBuffer = pQueue->beginCommands();

	// transition images from blit to render target
	TransitionImageBarrierBuilder<3> transition;
	for (uint32_t i = 0; i < transition.getCount(); ++i) {
		transition
			.setLayouts(i, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.setAccessMasks(i, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
			.setAspectMask(i, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	transition
		.setImage(0, m_GBuffer.colorImage->handle())
		.setImage(1, m_GBuffer.normalImage->handle())
		.setImage(2, m_GBuffer.depthImage->handle())
		.execute(m_renderCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = m_width;
	renderPassInfo.renderArea.extent.height = m_height;

	std::array<VkClearValue, 4> clearValues{};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].color = { 1.0f, 0.0f, 0.0f, 0.0f };
	clearValues[3].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_renderCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_width);
	viewport.height = static_cast<float>(m_height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_renderCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = m_width;
	scissor.extent.height = m_height;
	vkCmdSetScissor(m_renderCommandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { m_mesh->getVertexBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(m_renderCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(m_renderCommandBuffer, m_mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(m_renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(m_renderCommandBuffer, m_mesh->getIndexCount(), 1, 0, 0, 0);

	// the render pass will transition the framebuffer from render target to blit source
	vkCmdEndRenderPass(m_renderCommandBuffer);

	pQueue->endCommands(m_renderCommandBuffer);
}

void Application::recordBlitCommands() {
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_blitCommandBuffers.resize(m_device->getSwapChainImages().size());

	for (size_t i = 0; i < m_device->getSwapChainImages().size(); ++i) {
		VkCommandBuffer blitCommandBuffer = pQueue->beginCommands();
		m_blitCommandBuffers[i] = blitCommandBuffer;

		// transition the swapchain
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_device->getSwapChainImages()[i])
			.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.execute(blitCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { static_cast<int32_t>(m_width), static_cast<int32_t>(m_height), 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { static_cast<int32_t>(m_width), static_cast<int32_t>(m_height), 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = 0;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(blitCommandBuffer,
			//m_GBuffer.colorImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//m_GBuffer.normalImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//m_GBuffer.depthImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_raytracingImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_device->getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		// transition the swapchain again for ui rendering
		transition
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
			.execute(blitCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		pQueue->endCommands(blitCommandBuffer);
	}
}

void Application::recordRaytracingCommands() {
	Queue* pQueue = m_device->getQueue(QueueType::eGraphics);
	m_raytracingCommandBuffer = pQueue->beginCommands();

	// transition the raytracing output buffer from copy to storage
	TransitionImageBarrierBuilder<1> transition;
	transition
		.setImage(0, m_raytracingImage->handle())
		.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
		.setAccessMasks(0, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT)
		.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
		.execute(m_raytracingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	vkCmdBindPipeline(m_raytracingCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_raytracingPipeline);
	vkCmdBindDescriptorSets(m_raytracingCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_raytracingPipelineLayout, 0, 1, &m_raytracingDescriptorSet, 0, nullptr);

	m_device->getExtensions().vkCmdTraceRaysNV(m_raytracingCommandBuffer,
		m_shaderBindingTables.rgenShaderBindingTable.buffer, 0, // offset in the shader binding table for rgen
		m_shaderBindingTables.missShaderBindingTable.buffer, 0, m_shaderBindingTables.missShaderBindingTable.groupSize, // offset and stride for miss
		m_shaderBindingTables.chitShaderBindingTable.buffer, 0, m_shaderBindingTables.chitShaderBindingTable.groupSize, // offset and stride for chit
		nullptr, 0, 0,
		m_width, m_height, 1);
	
	// transition the raytracing image to be blitted
	transition
		.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT)
		.execute(m_raytracingCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	pQueue->endCommands(m_raytracingCommandBuffer);
}

}
