#include "Application.h"

#include "Builder/DescriptorSetBuilder.h"
#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/FramebufferBuilder.h"
#include "Builder/PipelineBuilder.h"
#include "Builder/PipelineLayoutBuilder.h"
#include "Builder/RaytracingPipelineBuilder.h"
#include "Builder/RenderPassBuilder.h"
#include "Builder/SamplerBuilder.h"

#include <chrono>
#include <iostream>
#include <vector>

// temporary
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

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
	, m_imageAvailableSemaphore{ VK_NULL_HANDLE }
	, m_renderFinishedSemaphore{ VK_NULL_HANDLE }
	, m_blitFinishedSemaphore{ VK_NULL_HANDLE }
	, m_inFlightFence{ VK_NULL_HANDLE }
	, m_blitFence{ VK_NULL_HANDLE }
	, m_renderCommandBuffer{ VK_NULL_HANDLE }
	, m_blitCommandBuffers()
	, m_raytracingImage{ nullptr }
	, m_raytracingImageView{ VK_NULL_HANDLE }
	, m_raytracingUniformBuffer{ nullptr }
	, m_raytracingDescriptorSetLayout{ VK_NULL_HANDLE }
	, m_raytracingPipelineLayout{ VK_NULL_HANDLE }
	, m_raytracingPipeline{ VK_NULL_HANDLE }
	, m_raytracingDescriptorSet{ VK_NULL_HANDLE }
	, m_accelerationStructures{}
	, m_raytracingCommandBuffer{ VK_NULL_HANDLE }
{
}

Application::~Application() {
	// TODO: wrap as many of those members into classes that know how to delete the Vulkan objects
	m_accelerationStructures.clean(m_device);
	vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_raytracingDescriptorSet);
	vkDestroyPipeline(m_device->handle(), m_raytracingPipeline, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_raytracingPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->handle(), m_raytracingDescriptorSetLayout, nullptr);
	delete m_raytracingUniformBuffer;
	vkDestroyImageView(m_device->handle(), m_raytracingImageView, nullptr);
	delete m_raytracingImage;

	m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(m_renderCommandBuffer);
	for (auto& cmd : m_blitCommandBuffers)
		m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(cmd);

	vkDestroySemaphore(m_device->handle(), m_blitFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device->handle(), m_renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device->handle(), m_imageAvailableSemaphore, nullptr);
	vkDestroyFence(m_device->handle(), m_inFlightFence, nullptr);
	vkDestroyFence(m_device->handle(), m_blitFence, nullptr);

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
	PipelineBuilder pipelineBuilder(m_device);
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

	// for raytracing, we should create some objects
	setupRaytracingData();

	// create the sync objects
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_blitFinishedSemaphore) != VK_SUCCESS) {

		std::cerr << "failed to create semaphores for a frame!" << std::endl;
		return false;
	}
	
	if (vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_blitFence) != VK_SUCCESS ||
		vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {

		std::cerr << "failed to create fences for a frame!" << std::endl;
		return false;
	}

	// record the commands that will be executed, rendering and blitting
	recordRenderCommands();
	recordBlitCommands();
	recordRaytracingCommands();

	return true;
}

void Application::run() {
	mainLoop();
}

void Application::notifyFramebufferResized(int width, int height) {
	m_framebufferResized = true;
	m_width = static_cast<uint32_t>(width);
	m_height = static_cast<uint32_t>(height);
}

void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // do not allow resizing for now

	m_width = static_cast<uint32_t>(WIDTH);
	m_height = static_cast<uint32_t>(HEIGHT);
	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Amano Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		drawFrame();
	}

	m_device->waitIdle();
}

void Application::drawFrame() {
	// NOTE: this isn't useful now since we are waiting for the previous frame to finish to render the next one
	vkWaitForFences(m_device->handle(), 1, &m_blitFence, VK_TRUE, UINT64_MAX);
	vkWaitForFences(m_device->handle(), 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_device->handle(), 1, &m_blitFence);
	vkResetFences(m_device->handle(), 1, &m_inFlightFence);

	uint32_t imageIndex;
	auto result = m_device->acquireNextImage(m_imageAvailableSemaphore, imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		std::cerr << "failed to acquire swap chain image!" << std::endl;
	}

	updateUniformBuffer();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	//submitInfo.commandBufferCount = 1;
	//submitInfo.pCommandBuffers = &m_renderCommandBuffer;
	std::array<VkCommandBuffer, 2> commandBuffers = { m_renderCommandBuffer, m_raytracingCommandBuffer };
	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	submitInfo.pCommandBuffers = commandBuffers.data();

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// submit rendering
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	if (!pQueue->submit(&submitInfo, m_blitFence))
		return;

	// submit blit
	VkSubmitInfo blitSubmitInfo{};
	blitSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore blitWaitSemaphores[] = { m_renderFinishedSemaphore };
	VkPipelineStageFlags blitWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	blitSubmitInfo.waitSemaphoreCount = 1;
	blitSubmitInfo.pWaitSemaphores = blitWaitSemaphores;
	blitSubmitInfo.pWaitDstStageMask = blitWaitStages;
	std::array<VkCommandBuffer, 1> commandBuffers2 = { m_blitCommandBuffers[imageIndex] };
	blitSubmitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers2.size());
	blitSubmitInfo.pCommandBuffers = commandBuffers2.data();

	VkSemaphore signalSemaphores2[] = { m_blitFinishedSemaphore };
	blitSubmitInfo.signalSemaphoreCount = 1;
	blitSubmitInfo.pSignalSemaphores = signalSemaphores2;

	if (!pQueue->submit(&blitSubmitInfo, m_inFlightFence))
		return;
	
	// wait for the rendering to finish
	m_device->presentAndWait(m_blitFinishedSemaphore, imageIndex);
}

void Application::updateUniformBuffer() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	
	glm::vec3 origin = glm::vec3(2.8f * cosf(time), 2.8f * sinf(time), 2.0f);

	PerFrameUniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(origin, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_width / (float)m_height, 0.1f, 10.0f);

	// TODO: fix this
	ubo.proj[1][1] *= -1;

	m_uniformBuffer->update(ubo);

	RayParams rayUbo{};
	rayUbo.viewInverse = glm::inverse(ubo.view);
	rayUbo.projInverse = glm::inverse(ubo.proj);
	rayUbo.rayOrigin = origin;

	m_raytracingUniformBuffer->update(rayUbo);
}

void Application::recordRenderCommands() {
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_renderCommandBuffer = pQueue->beginCommands();
	
	VkImageMemoryBarrier barriers[2];
	for (int i = 0; i < 2; ++i) {
		barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[i].pNext = nullptr;
		barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barriers[i].subresourceRange.baseArrayLayer = 0;
		barriers[i].subresourceRange.layerCount = 1;
		barriers[i].subresourceRange.levelCount = 1;
		barriers[i].subresourceRange.baseMipLevel = 0;
		barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[i].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barriers[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	barriers[0].image = m_colorImage->handle();
	barriers[1].image = m_normalImage->handle();

	vkCmdPipelineBarrier(m_renderCommandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		0, nullptr,
		0, nullptr,
		2, barriers);
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = m_width;
	renderPassInfo.renderArea.extent.height = m_height;

	std::array<VkClearValue, 3> clearValues{};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_renderCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkBuffer vertexBuffers[] = { m_model->getVertexBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(m_renderCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(m_renderCommandBuffer, m_model->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(m_renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(m_renderCommandBuffer, m_model->getIndexCount(), 1, 0, 0, 0);

	vkCmdEndRenderPass(m_renderCommandBuffer);

	pQueue->endCommands(m_renderCommandBuffer);
}

void Application::recordBlitCommands() {
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_blitCommandBuffers.resize(m_device->getSwapChainImages().size());

	for (size_t i = 0; i < m_device->getSwapChainImages().size(); ++i) {
		VkCommandBuffer blitCommandBuffer = pQueue->beginCommands();
		m_blitCommandBuffers[i] = blitCommandBuffer;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.image = m_device->getSwapChainImages()[i];
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(blitCommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

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
			//m_colorImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//m_normalImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_raytracingImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_device->getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		vkCmdPipelineBarrier(blitCommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		pQueue->endCommands(blitCommandBuffer);
	}
}

void Application::setupRaytracingData() {
	// this is the buffer storing the result of the raytracing
	m_raytracingImage = new Image(m_device);
	m_raytracingImage->create(
		m_width,
		m_height,
		1,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_raytracingImageView = m_raytracingImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

	// uniform buffer
	m_raytracingUniformBuffer = new UniformBuffer<RayParams>(m_device);
	
	// create layout for the raytracing pipeline
	DescriptorSetLayoutBuilder descriptorSetLayoutbuilder;
	descriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, VK_SHADER_STAGE_RAYGEN_BIT_NV)
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV)
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);
	m_raytracingDescriptorSetLayout = descriptorSetLayoutbuilder.build(*m_device);

	// create raytracing pipeline layout
	PipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayoutBuilder.addDescriptorSetLayout(m_raytracingDescriptorSetLayout);
	m_raytracingPipelineLayout = pipelineLayoutBuilder.build(*m_device);

	RaytracingPipelineBuilder raytracingPipelineBuilder(m_device);
	raytracingPipelineBuilder
		.addShader("../../compiled_shaders/raygen.spv", VK_SHADER_STAGE_RAYGEN_BIT_NV)
		.addShader("../../compiled_shaders/miss.spv", VK_SHADER_STAGE_MISS_BIT_NV)
		.addShader("../../compiled_shaders/closesthit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	m_raytracingPipeline = raytracingPipelineBuilder.build(m_raytracingPipelineLayout, 1);

	RaytracingAccelerationStructureBuilder accelerationStructureBuilder(m_device);
	accelerationStructureBuilder
		.addRayGenShader(0)
		.addMissShader(1)
		.addClosestHitShader(2);
	m_accelerationStructures = accelerationStructureBuilder.build(*m_model, m_raytracingPipeline);

	// update the descriptor set
	DescriptorSetBuilder descriptorSetBuilder(m_device, 2, m_raytracingDescriptorSetLayout);
	descriptorSetBuilder
		.addAccelerationStructure(&m_accelerationStructures.top, 0)
		.addStorageImage(m_raytracingImageView, 1)
		.addUniformBuffer(m_raytracingUniformBuffer->getBuffer(), sizeof(RayParams), 2);
	m_raytracingDescriptorSet = descriptorSetBuilder.buildAndUpdate();
}

void Application::recordRaytracingCommands() {
	Queue* pQueue = m_device->getQueue(QueueType::eGraphics);
	m_raytracingCommandBuffer = pQueue->beginCommands();

	VkImageMemoryBarrier barrier0{};
	barrier0.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier0.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier0.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier0.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier0.image = m_raytracingImage->handle();
	barrier0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier0.subresourceRange.baseMipLevel = 0;
	barrier0.subresourceRange.levelCount = 1;
	barrier0.subresourceRange.baseArrayLayer = 0;
	barrier0.subresourceRange.layerCount = 1;
	barrier0.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // TODO
	barrier0.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // TODO


	vkCmdPipelineBarrier(
		m_raytracingCommandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier0);

	vkCmdBindPipeline(m_raytracingCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_raytracingPipeline);
	vkCmdBindDescriptorSets(m_raytracingCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_raytracingPipelineLayout, 0, 1, &m_raytracingDescriptorSet, 0, nullptr);

	m_device->getExtensions().vkCmdTraceRaysNV(m_raytracingCommandBuffer,
		m_accelerationStructures.bindingTable.shaderBindingTableBuffer, m_accelerationStructures.raytracingShaderGroupIndices.rgenGroupOffset, // offset in the shader binding table for rgen
		m_accelerationStructures.bindingTable.shaderBindingTableBuffer, m_accelerationStructures.raytracingShaderGroupIndices.missGroupOffset, m_accelerationStructures.raytracingShaderGroupIndices.missGroupSize, // offset and stride for miss
		m_accelerationStructures.bindingTable.shaderBindingTableBuffer, m_accelerationStructures.raytracingShaderGroupIndices.chitGroupOffset, m_accelerationStructures.raytracingShaderGroupIndices.chitGroupSize, // offset and stride for hit (2 shaders)
		nullptr, 0, 0,
		m_width, m_height, 1);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_raytracingImage->handle();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // TODO
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // TODO


	vkCmdPipelineBarrier(
		m_raytracingCommandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	pQueue->endCommands(m_raytracingCommandBuffer);
}

}
