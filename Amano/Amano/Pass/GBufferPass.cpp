#include "GBufferPass.h"
#include "../Builder/DescriptorSetBuilder.h"
#include "../Builder/DescriptorSetLayoutBuilder.h"
#include "../Builder/FramebufferBuilder.h"
#include "../Builder/GraphicsPipelineBuilder.h"
#include "../Builder/PipelineLayoutBuilder.h"
#include "../Builder/RenderPassBuilder.h"
#include "../Builder/SamplerBuilder.h"
#include "../Builder/TransitionImageBarrierBuilder.h"

#include <iostream>

namespace Amano {

GBufferPass::GBufferPass(Device* device)
	: Pass(device, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV)
	, m_descriptorSetLayout{ VK_NULL_HANDLE }
	, m_pipelineLayout{ VK_NULL_HANDLE }
	, m_pipeline{ VK_NULL_HANDLE }
	, m_renderPass{ VK_NULL_HANDLE }
	, m_descriptorSet{ VK_NULL_HANDLE }
	, m_framebuffer{ VK_NULL_HANDLE }
	, m_uniformBuffer(device)
	, m_albedoImage{ nullptr }
	, m_normalImage{ nullptr }
	, m_depthImage{ nullptr }
	, m_commandBuffer{ VK_NULL_HANDLE }
{
}

GBufferPass::~GBufferPass() {
	cleanOnRenderTargetResized();

	vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
	vkDestroyRenderPass(m_device->handle(), m_renderPass, nullptr);
}

bool GBufferPass::init() {
	Formats formats = getFormats();

	// create the render pass
	RenderPassBuilder renderPassBuilder;
	renderPassBuilder
		.addColorAttachment(formats.colorFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // attachment 0 for color
		.addColorAttachment(formats.normalFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // attachment 1 for normal
		.addDepthAttachment(formats.depthFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) // attachment 2 for depth buffer
		.addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, { 0, 1 }, 2) // subpass 0
		.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0);
	m_renderPass = renderPassBuilder.build(*m_device);

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
	GraphicsPipelineBuilder pipelineBuilder(m_device);
	pipelineBuilder
		.addShader("compiled_shaders/gbufferv.spv", VK_SHADER_STAGE_VERTEX_BIT)
		.addShader("compiled_shaders/gbufferf.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		.setRasterizer(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	m_pipeline = pipelineBuilder.build(m_pipelineLayout, m_renderPass, 0, 2, true);

	return true;
}

void GBufferPass::recordCommands(uint32_t width, uint32_t height, const Mesh* mesh) {
	destroyCommandBuffer();

	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_commandBuffer = pQueue->beginCommands();

	// transition images from shader sampler to render target
	TransitionImageBarrierBuilder<3> transition;
	transition
		.setImage(0, m_albedoImage->handle())
		.setLayouts(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.setAccessMasks(0, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
		.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
		.setImage(1, m_normalImage->handle())
		.setLayouts(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		.setAccessMasks(1, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
		.setAspectMask(1, VK_IMAGE_ASPECT_COLOR_BIT)
		.setImage(2, m_depthImage->handle())
		.setLayouts(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		.setAccessMasks(2, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
		.setAspectMask(2, VK_IMAGE_ASPECT_DEPTH_BIT)
		.execute(m_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = width;
	renderPassInfo.renderArea.extent.height = height;

	std::array<VkClearValue, 4> clearValues{};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].color = { 1.0f, 0.0f, 0.0f, 0.0f };
	clearValues[3].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { mesh->getVertexBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(m_commandBuffer, mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(m_commandBuffer, mesh->getIndexCount(), 1, 0, 0, 0);

	// the render pass will transition the framebuffer from render target to shader sample
	vkCmdEndRenderPass(m_commandBuffer);

	pQueue->endCommands(m_commandBuffer);
}

void GBufferPass::cleanOnRenderTargetResized() {
	destroyDescriptorSet();
	destroyCommandBuffer();
	destroyGBufferImages();
}

void GBufferPass::recreateOnRenderTargetResized(uint32_t width, uint32_t height, const Mesh* mesh, Image* texture) {
	createGBufferImages(width, height);
	createDescriptorSet(texture);
	recordCommands(width, height, mesh);
}

void GBufferPass::updateUniformBuffer(PerFrameUniformBufferObject& ubo) {
	m_uniformBuffer.update(ubo);
}

bool GBufferPass::submit() {
	if (m_commandBuffer == VK_NULL_HANDLE)
		return false;

	// submit rendering
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores.size());
	submitInfo.pWaitSemaphores = m_waitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_waitPipelineStages.data();

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_signalSemaphore;

	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	if (!pQueue->submit(&submitInfo, VK_NULL_HANDLE))
		return false;

	return true;
}

void GBufferPass::createGBufferImages(uint32_t width, uint32_t height) {
	Formats formats = getFormats();
	auto pQueue = m_device->getQueue(QueueType::eGraphics);

	// create the depth image/buffer
	m_depthImage = new Image(m_device);
	m_depthImage->create2D(
		width,
		height,
		1,
		formats.depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_depthImage->createView(VK_IMAGE_ASPECT_DEPTH_BIT);
	m_depthImage->transitionLayout(*pQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// create the images for the GBuffer
	m_albedoImage = new Image(m_device);
	m_albedoImage->create2D(
		width,
		height,
		1,
		formats.colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_albedoImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);
	m_albedoImage->transitionLayout(*pQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_normalImage = new Image(m_device);
	m_normalImage->create2D(
		width,
		height,
		1,
		formats.normalFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_normalImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);
	m_normalImage->transitionLayout(*pQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// create the framebuffer
	FramebufferBuilder framebufferBuilder;
	framebufferBuilder
		.addAttachment(m_albedoImage->viewHandle())
		.addAttachment(m_normalImage->viewHandle())
		.addAttachment(m_depthImage->viewHandle());
	m_framebuffer = framebufferBuilder.build(*m_device, m_renderPass, width, height);
}

void GBufferPass::destroyGBufferImages() {
	delete m_albedoImage;
	delete m_normalImage;
	delete m_depthImage;
	m_albedoImage = nullptr;
	m_normalImage = nullptr;
	m_depthImage = nullptr;

	if (m_framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(m_device->handle(), m_framebuffer, nullptr);
		m_framebuffer = VK_NULL_HANDLE;
	}
}

bool GBufferPass::createDescriptorSet(Image* texture) {

	// update the descriptor set
	DescriptorSetBuilder descriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
	descriptorSetBuilder
		.addUniformBuffer(m_uniformBuffer.getBuffer(), m_uniformBuffer.getSize(), 0)
		.addImage(texture->sampler(), texture->viewHandle(), 1);
	m_descriptorSet = descriptorSetBuilder.buildAndUpdate();

	return m_descriptorSet != VK_NULL_HANDLE;
}

void GBufferPass::destroyDescriptorSet() {
	if (m_descriptorSet != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSet);
		m_descriptorSet = VK_NULL_HANDLE;
	}
}

void GBufferPass::destroyCommandBuffer() {
	if (m_commandBuffer != VK_NULL_HANDLE) {
		m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(m_commandBuffer);
		m_commandBuffer = VK_NULL_HANDLE;
	}
}

GBufferPass::Formats GBufferPass::getFormats() {
	Formats formats;
	formats.depthFormat = m_device->findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
	);
	formats.colorFormat = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_SRGB
	formats.normalFormat = VK_FORMAT_R16G16B16A16_SNORM;
	return formats;
}

}
