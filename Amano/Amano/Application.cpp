#include "Application.h"

#include "Builder/DescriptorSetBuilder.h"
#include "Builder/DescriptorSetLayoutBuilder.h"
#include "Builder/FramebufferBuilder.h"
#include "Builder/PipelineBuilder.h"
#include "Builder/PipelineLayoutBuilder.h"
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
	, m_imageAvailableSemaphores()
	, m_renderFinishedSemaphores()
	, m_blitFinishedSemaphores()
	, m_inFlightFences()
	, m_blitFences()
	, m_imagesInFlight()
	, m_renderCommandBuffer{ VK_NULL_HANDLE }
	, m_blitCommandBuffers()
	, m_currentFrame{ 0 }
{
}

Application::~Application() {
	// TODO: wrap as many of those members into classes that know how to delete the Vulkan objects
	m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(m_renderCommandBuffer);
	for (auto& cmd : m_blitCommandBuffers)
		m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(cmd);

	for (size_t i = 0; i < m_device->getSwapChainImages().size(); ++i) {
		vkDestroySemaphore(m_device->handle(), m_blitFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_device->handle(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_device->handle(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_device->handle(), m_inFlightFences[i], nullptr);
		vkDestroyFence(m_device->handle(), m_blitFences[i], nullptr);
	}

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

	// create the sync objects
	
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_blitFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_blitFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(m_device->getSwapChainImages().size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_blitFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_blitFences[i]) != VK_SUCCESS ||
			vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {

			std::cerr << "failed to create synchronization objects for a frame!" << std::endl;
			return false;
		}
	}

	recordRenderCommands();
	recordBlitCommands();

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
	vkWaitForFences(m_device->handle(), 1, &m_inFlightFences[(m_currentFrame + 1) % 2], VK_TRUE, UINT64_MAX);
	vkWaitForFences(m_device->handle(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_device->handle(), 1, &m_inFlightFences[m_currentFrame]);
	vkResetFences(m_device->handle(), 1, &m_blitFences[m_currentFrame]);

	uint32_t imageIndex;
	auto result = m_device->acquireNextImage(m_imageAvailableSemaphores[m_currentFrame], imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		//recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		std::cerr << "failed to acquire swap chain image!" << std::endl;
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(m_device->handle(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	m_imagesInFlight[imageIndex] = m_imagesInFlight[m_currentFrame];

	updateUniformBuffer();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_renderCommandBuffer;

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_device->handle(), 1, &m_inFlightFences[m_currentFrame]);
	vkResetFences(m_device->handle(), 1, &m_blitFences[m_currentFrame]);

	// submit rendering
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	if (!pQueue->submit(&submitInfo, m_blitFences[m_currentFrame]))
		return;

	// submit blit
	VkSubmitInfo blitSubmitInfo{};
	blitSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore blitWaitSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	VkPipelineStageFlags blitWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	blitSubmitInfo.waitSemaphoreCount = 1;
	blitSubmitInfo.pWaitSemaphores = blitWaitSemaphores;
	blitSubmitInfo.pWaitDstStageMask = blitWaitStages;
	std::array<VkCommandBuffer, 1> commandBuffers2 = { m_blitCommandBuffers[imageIndex] };
	blitSubmitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers2.size());
	blitSubmitInfo.pCommandBuffers = commandBuffers2.data();

	VkSemaphore signalSemaphores2[] = { m_blitFinishedSemaphores[m_currentFrame] };
	blitSubmitInfo.signalSemaphoreCount = 1;
	blitSubmitInfo.pSignalSemaphores = signalSemaphores2;

	if (!pQueue->submit(&blitSubmitInfo, m_inFlightFences[m_currentFrame]))
		return;
	
	// wait for the rendering to finish
	m_device->presentAndWait(m_blitFinishedSemaphores[m_currentFrame], imageIndex);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::updateUniformBuffer() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	
	PerFrameUniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_width / (float)m_height, 0.1f, 10.0f);

	// TODO: fix this
	ubo.proj[1][1] *= -1;

	m_uniformBuffer->update(ubo);
}

void Application::recordRenderCommands() {
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_renderCommandBuffer = pQueue->beginCommands();

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

	vkCmdDrawIndexed(m_renderCommandBuffer, m_model->getIndiceCount(), 1, 0, 0, 0);

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
			m_colorImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//m_normalImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

		m_device->getQueue(QueueType::eGraphics)->endCommands(blitCommandBuffer);
	}
}

}
