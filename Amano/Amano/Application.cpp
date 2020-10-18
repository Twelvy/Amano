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
	// necessary information to display the model
	, m_mesh{ nullptr }
	, m_modelTexture{ nullptr }
	, m_imageAvailableSemaphore{ VK_NULL_HANDLE }
	, m_inFlightFence{ VK_NULL_HANDLE }
	, m_gBufferPass{ nullptr }
	, m_deferredLightingPass{ nullptr }
	, m_raytracingPass{ nullptr }
	, m_toneMappingPass{ nullptr }
	, m_blitToSwapChainPass{ nullptr }
	// light information
	, m_lightPosition(1.0f, 1.0f, 1.0f)
{
}

Application::~Application() {
	// TODO: wrap as many of those members into classes that know how to delete the Vulkan objects
	cleanSizedependentObjects();

	delete m_blitToSwapChainPass;
	delete m_toneMappingPass;
	delete m_raytracingPass;
	delete m_deferredLightingPass;
	delete m_gBufferPass;

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

		m_gBufferPass->recreateOnRenderTargetResized(m_width, m_height, m_mesh, m_modelTexture);
		m_deferredLightingPass->recreateOnRenderTargetResized(m_width, m_height, m_gBufferPass->albedoImage(), m_gBufferPass->normalImage(), m_gBufferPass->depthImage());

		if (m_raytracingPass != nullptr)
			m_raytracingPass->recreateOnRenderTargetResized(m_width, m_height, m_gBufferPass->depthImage(), m_gBufferPass->normalImage(), m_deferredLightingPass->outputImage());
#ifdef AMANO_USE_RAYTRACING
		m_toneMappingPass->recreateOnRenderTargetResized(m_width, m_height, m_raytracingPass->outputImage());
#else
		m_toneMappingPass->recreateOnRenderTargetResized(m_width, m_height, m_deferredLightingPass->outputImage());
#endif
		m_blitToSwapChainPass->recreateOnRenderTargetResized(m_width, m_height, m_toneMappingPass->outputImage());
		m_guiSystem->recreateOnRenderTargetResized(m_width, m_height);
	}
}

void Application::cleanSizedependentObjects() {
	if (m_gBufferPass != nullptr)
		m_gBufferPass->cleanOnRenderTargetResized();
	if (m_deferredLightingPass != nullptr)
		m_deferredLightingPass->cleanOnRenderTargetResized();
	if (m_raytracingPass != nullptr)
		m_raytracingPass->cleanOnRenderTargetResized();
	if (m_toneMappingPass != nullptr)
		m_toneMappingPass->cleanOnRenderTargetResized();
	if (m_blitToSwapChainPass != nullptr)
		m_blitToSwapChainPass->cleanOnRenderTargetResized();
	if (m_guiSystem != nullptr)
		m_guiSystem->cleanOnRenderTargetResized();
}

bool Application::init() {
	initWindow();

	m_device = new Device();
	if (!m_device->init(m_window)) return false;

	/////////////////////////////////////////////
	// from here, this is a test application
	/////////////////////////////////////////////

	// create the sync objects
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS) {

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
	m_modelTexture->create2D("assets/textures/white.png", *m_device->getQueue(QueueType::eGraphics), true);
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
#ifdef AMANO_USE_RAYTRACING
	std::vector<Mesh*> meshes;
	meshes.push_back(m_mesh);
	m_raytracingPass = new RaytracingShadowPass(m_device);
	m_raytracingPass->addWaitSemaphore(m_deferredLightingPass->signalSemaphore(), m_deferredLightingPass->pipelineStage());
	if (!m_raytracingPass->init(meshes))
		return false;
#endif

	/////////////////////////////////////////////
	// Tone mapping
	/////////////////////////////////////////////
	m_toneMappingPass = new ToneMappingPass(m_device);
#ifdef AMANO_USE_RAYTRACING
	m_toneMappingPass->addWaitSemaphore(m_raytracingPass->signalSemaphore(), m_raytracingPass->pipelineStage());
#else
	m_toneMappingPass->addWaitSemaphore(m_deferredLightingPass->signalSemaphore(), m_deferredLightingPass->pipelineStage());
#endif
	if (!m_toneMappingPass->init())
		return false;

	/////////////////////////////////////////////
	// Blit
	/////////////////////////////////////////////
	m_blitToSwapChainPass = new BlitToSwapChainPass(m_device);
	m_blitToSwapChainPass->addWaitSemaphore(m_toneMappingPass->signalSemaphore(), m_toneMappingPass->pipelineStage());
	
	/////////////////////////////////////////////
	// UI
	/////////////////////////////////////////////
	m_guiSystem = new ImGuiSystem(m_device);
	m_guiSystem->addWaitSemaphore(m_blitToSwapChainPass->signalSemaphore(), m_blitToSwapChainPass->pipelineStage());
	if (!m_guiSystem->init()) return false;

	m_debugOrbitCamera = new DebugOrbitCamera();

	m_inputSystem = new InputSystem();
	m_inputSystem->registerReader(m_guiSystem);
	m_inputSystem->registerReader(m_debugOrbitCamera);

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
	if (m_raytracingPass != nullptr && !m_raytracingPass->submit())
		return;

	// submit tone mapping
	if (!m_toneMappingPass->submit())
		return;

	// submit blit
	if (!m_blitToSwapChainPass->submit(imageIndex, VK_NULL_HANDLE))
		return;

	// udpate UI
	drawUI(imageIndex);
	
	// TODO: this should be the UI semaphore
	// wait for the rendering to finish
	result = m_device->present(m_guiSystem->signalSemaphore(), imageIndex);

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

	m_guiSystem->endFrame(imageIndex, m_width, m_height, m_inFlightFence);
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

}
