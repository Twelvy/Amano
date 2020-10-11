#include "Application.h"

#include "Builder/DescriptorSetBuilder.h"
#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/FramebufferBuilder.h"
#include "Builder/GraphicsPipelineBuilder.h"
#include "Builder/ComputePipelineBuilder.h"
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
	, m_mesh{ nullptr }
	, m_modelTexture{ nullptr }
	, m_imageAvailableSemaphore{ VK_NULL_HANDLE }
	, m_blitFinishedSemaphore{ VK_NULL_HANDLE }
	, m_inFlightFence{ VK_NULL_HANDLE }
	, m_blitCommandBuffers()
	, m_gBufferPass{ nullptr }
	// deferred lighting
	, m_deferredLightingPass{ nullptr }
	// raytracing
	, m_raytracingPass{ nullptr }
	// light information
	, m_lightPosition(1.0f, 1.0f, 1.0f)
{
}

Application::~Application() {
	// TODO: wrap as many of those members into classes that know how to delete the Vulkan objects
	cleanSizedependentObjects();

	delete m_raytracingPass;
	delete m_deferredLightingPass;
	delete m_gBufferPass;

	vkDestroySemaphore(m_device->handle(), m_blitFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device->handle(), m_imageAvailableSemaphore, nullptr);
	vkDestroyFence(m_device->handle(), m_inFlightFence, nullptr);

	delete m_modelTexture;
	delete m_mesh;
	
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

		// record the commands that will be executed, rendering and blitting
		m_gBufferPass->recreateOnRenderTargetResized(m_width, m_height, m_mesh, m_modelTexture);
		m_deferredLightingPass->recreateOnRenderTargetResized(m_width, m_height, m_gBufferPass->albedoImage(), m_gBufferPass->normalImage(), m_gBufferPass->depthImage());
		m_raytracingPass->recreateOnRenderTargetResized(m_width, m_height, m_gBufferPass->depthImage(), m_gBufferPass->normalImage(), m_deferredLightingPass->outputImage());

		recordBlitCommands();
	}
}

void Application::cleanSizedependentObjects() {
	for (auto& cmd : m_blitCommandBuffers)
		m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(cmd);
	m_blitCommandBuffers.clear();

	for (auto finalFb : m_finalFramebuffers)
		vkDestroyFramebuffer(m_device->handle(), finalFb, nullptr);
	m_finalFramebuffers.clear();

	if (m_gBufferPass != nullptr)
		m_gBufferPass->cleanOnRenderTargetResized();
	if (m_deferredLightingPass != nullptr)
		m_deferredLightingPass->cleanOnRenderTargetResized();
	if (m_raytracingPass != nullptr)
		m_raytracingPass->cleanOnRenderTargetResized();
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

	// create the sync objects
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_blitFinishedSemaphore) != VK_SUCCESS) {

		std::cerr << "failed to create semaphores for a frame!" << std::endl;
		return false;
	}

	if (vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {
		std::cerr << "failed to create fences for a frame!" << std::endl;
		return false;
	}

	
	// load the model to display
	m_mesh = new Mesh(m_device);
	m_mesh->create("assets/models/sphere.obj");

	// load the texture of the model
	m_modelTexture = new Image(m_device);
	m_modelTexture->create2D("assets/textures/white.png", *m_device->getQueue(QueueType::eGraphics));
	m_modelTexture->createView(VK_IMAGE_ASPECT_COLOR_BIT);
	m_modelTexture->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

	/////////////////////////////////////////////
	// GBuffer pass
	/////////////////////////////////////////////
	m_gBufferPass = new GBufferPass(m_device);
	m_gBufferPass->addWaitSemaphore(m_imageAvailableSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT); // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
	if (!m_gBufferPass->init())
		return false;

	/////////////////////////////////////////////
	// Deferred lighting
	/////////////////////////////////////////////
	m_deferredLightingPass = new DeferredLightingPass(m_device);
	m_deferredLightingPass->addWaitSemaphore(m_gBufferPass->signalSemaphore(), m_gBufferPass->pipelineStage());
	if (!m_deferredLightingPass->init())
		return false;

	/////////////////////////////////////////////
	// Raytracing
	/////////////////////////////////////////////

	std::vector<Mesh*> meshes;
	meshes.push_back(m_mesh);
	m_raytracingPass = new RaytracingShadowPass(m_device);
	m_raytracingPass->addWaitSemaphore(m_deferredLightingPass->signalSemaphore(), m_deferredLightingPass->pipelineStage());
	if (!m_raytracingPass->init(meshes))
		return false;

	/////////////////////////////////////////////
	// Create all the object which depend on the
	// size of the final render target
	/////////////////////////////////////////////
	// This is a very shaky implmentation for now
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
	//vkWaitForFences(m_device->handle(), 1, &m_blitFence, VK_TRUE, UINT64_MAX);
	vkWaitForFences(m_device->handle(), 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
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
	updateUniformBuffers();

	// submit GBuffer
	if (!m_gBufferPass->submit())
		return;

	// submit deferred lighting
	if (!m_deferredLightingPass->submit())
		return;

	// submit raytracing
	if (!m_raytracingPass->submit())
		return;

	// submit blit
	// 1. wait for the raytracing finished semaphore
	// 2. signal the blit finished semaphore
	// 3. notify the in-flight fence
	VkSubmitInfo blitSubmitInfo{};
	blitSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore blitWaitSemaphores[] = { m_raytracingPass->signalSemaphore() };
	VkPipelineStageFlags blitWaitStages[] = { m_raytracingPass->pipelineStage() };
	blitSubmitInfo.waitSemaphoreCount = 1;
	blitSubmitInfo.pWaitSemaphores = blitWaitSemaphores;
	blitSubmitInfo.pWaitDstStageMask = blitWaitStages;
	blitSubmitInfo.commandBufferCount = 1;
	blitSubmitInfo.pCommandBuffers = &m_blitCommandBuffers[imageIndex];

	VkSemaphore blitSignalSemaphores[] = { m_blitFinishedSemaphore };
	blitSubmitInfo.signalSemaphoreCount = 1;
	blitSubmitInfo.pSignalSemaphores = blitSignalSemaphores;

	auto pQueue = m_device->getQueue(QueueType::eGraphics);
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

void Application::updateUniformBuffers() {
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

	// update the raytracing shader uniform
	RayParams rayUbo{};
	rayUbo.viewInverse = glm::inverse(ubo.view);
	rayUbo.projInverse = glm::inverse(ubo.proj);
	rayUbo.rayOrigin = origin;

	// update the light position
	LightInformation lightUbo;
	lightUbo.lightPosition = m_lightPosition;

	if (m_gBufferPass != nullptr) {
		m_gBufferPass->updateUniformBuffer(ubo);
	}

	if (m_deferredLightingPass != nullptr) {
		m_deferredLightingPass->updateUniformBuffer(rayUbo);
		m_deferredLightingPass->updateLightUniformBuffer(lightUbo);
	}

	if (m_raytracingPass != nullptr) {
		m_raytracingPass->updateRayUniformBuffer(rayUbo);
		m_raytracingPass->updateLightUniformBuffer(lightUbo);
	}
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
			m_raytracingPass->outputImage()->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

}
