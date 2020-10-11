#include "DeferredLightingPass.h"
#include "../Builder/ComputePipelineBuilder.h"
#include "../Builder/DescriptorSetBuilder.h"
#include "../Builder/DescriptorSetLayoutBuilder.h"
#include "../Builder/PipelineLayoutBuilder.h"
#include "../Builder/SamplerBuilder.h"
#include "../Builder/TransitionImageBarrierBuilder.h"

namespace Amano {

DeferredLightingPass::DeferredLightingPass(Device* device)
	: Pass(device, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
	, m_descriptorSetLayout{ VK_NULL_HANDLE }
	, m_pipelineLayout{ VK_NULL_HANDLE }
	, m_pipeline{ VK_NULL_HANDLE }
	, m_descriptorSet{ VK_NULL_HANDLE }
	, m_nearestSampler{ VK_NULL_HANDLE }
	, m_linearSampler{ VK_NULL_HANDLE }
	, m_uniformBuffer(device)
	, m_lightUniformBuffer(device)
	, m_outputImage{ nullptr }
	, m_environmentImage{ nullptr }
	, m_commandBuffer{ VK_NULL_HANDLE }
{
}

DeferredLightingPass::~DeferredLightingPass() {
	cleanOnRenderTargetResized();

	vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
	vkDestroySampler(m_device->handle(), m_nearestSampler, nullptr);
	vkDestroySampler(m_device->handle(), m_linearSampler, nullptr);
	delete m_environmentImage;
}

bool DeferredLightingPass::init() {
	m_environmentImage = new Image(m_device);
	m_environmentImage->createCube(
		"assets/textures/Yokohama3/posx.jpg",
		"assets/textures/Yokohama3/negx.jpg",
		"assets/textures/Yokohama3/posy.jpg",
		"assets/textures/Yokohama3/negy.jpg",
		"assets/textures/Yokohama3/posz.jpg",
		"assets/textures/Yokohama3/negz.jpg",
		*m_device->getQueue(QueueType::eGraphics));
	m_environmentImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);

	DescriptorSetLayoutBuilder computeDescriptorSetLayoutbuilder;
	computeDescriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // albedo image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // normal image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // depth image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // environment image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)    // camera information
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)    // light information
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);   // output image
	m_descriptorSetLayout = computeDescriptorSetLayoutbuilder.build(*m_device);

	// create raytracing pipeline layout
	PipelineLayoutBuilder computePipelineLayoutBuilder;
	computePipelineLayoutBuilder.addDescriptorSetLayout(m_descriptorSetLayout);
	m_pipelineLayout = computePipelineLayoutBuilder.build(*m_device);

	// load the shaders and create the pipeline
	ComputePipelineBuilder computePipelineBuilder(m_device);
	computePipelineBuilder
		.addShader("compiled_shaders/deferred_lighting.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	m_pipeline = computePipelineBuilder.build(m_pipelineLayout);

	// create a sampler for the textures
	SamplerBuilder nearestSamplerBuilder;
	nearestSamplerBuilder
		.setMaxLoad(1)
		.setFilter(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
	m_nearestSampler = nearestSamplerBuilder.build(*m_device);

	// create a sampler for the environment cubemap
	SamplerBuilder linearSamplerBuilder;
	linearSamplerBuilder.setMaxLoad((float)m_environmentImage->getMipLevels());
	m_linearSampler = linearSamplerBuilder.build(*m_device);

	return true;
}

void DeferredLightingPass::cleanOnRenderTargetResized() {
	destroyDescriptorSet();
	destroyCommandBuffer();
	destroyOutputImage();
}

void DeferredLightingPass::recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* albedoImage, Image* normalImage, Image* depthImage) {
	createOutputImage(width, height);
	createDescriptorSet(albedoImage, normalImage, depthImage);
	recordCommands(width, height);
}

void DeferredLightingPass::updateUniformBuffer(RayParams& ubo) {
	m_uniformBuffer.update(ubo);
}

void DeferredLightingPass::updateLightUniformBuffer(LightInformation& ubo) {
	m_lightUniformBuffer.update(ubo);
}

void DeferredLightingPass::recordCommands(uint32_t width, uint32_t height) {
	destroyCommandBuffer();

	Queue* pQueue = m_device->getQueue(QueueType::eCompute);
	m_commandBuffer = pQueue->beginCommands();

	TransitionImageBarrierBuilder<1> transition;
	transition
		.setImage(0, m_outputImage->handle())
		.setLayouts(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL)
		.setAccessMasks(0, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT)
		.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
		.execute(m_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	// local size is 32 for x and y
	const uint32_t locaSizeX = 32;
	const uint32_t locaSizeY = 32;
	uint32_t dispatchX = (width + locaSizeX - 1) / locaSizeX;
	uint32_t dispatchY = (height + locaSizeY - 1) / locaSizeY;
	vkCmdDispatch(m_commandBuffer, dispatchX, dispatchY, 1);

	// transition the compute image to shader sampler
	transition
		.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
		.execute(m_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	pQueue->endCommands(m_commandBuffer);
}

bool DeferredLightingPass::submit() {
	// submit lighting compute
	// 1. wait for the semaphores
	// 2. signal the render finished semaphore
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores.size());
	submitInfo.pWaitSemaphores = m_waitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_waitPipelineStages.data();;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_signalSemaphore;

	auto pComputeQueue = m_device->getQueue(QueueType::eCompute);
	if (!pComputeQueue->submit(&submitInfo, VK_NULL_HANDLE))
		return false;

	return true;
}

void DeferredLightingPass::createOutputImage(uint32_t width, uint32_t height) {
	m_outputImage = new Image(m_device);
	m_outputImage->create2D(
		width,
		height,
		1,
		VK_FORMAT_R8G8B8A8_UNORM, //formats.colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_outputImage->createView(VK_IMAGE_ASPECT_COLOR_BIT);
	m_outputImage->transitionLayout(*m_device->getQueue(QueueType::eCompute), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void DeferredLightingPass::destroyOutputImage() {
	delete m_outputImage;
	m_outputImage = nullptr;
}

bool DeferredLightingPass::createDescriptorSet(Image* albedoImage, Image* normalImage, Image* depthImage) {
	// update the descriptor set
	DescriptorSetBuilder computeDescriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
	computeDescriptorSetBuilder
		.addImage(m_nearestSampler, albedoImage->viewHandle(), 0)
		.addImage(m_nearestSampler, normalImage->viewHandle(), 1)
		.addImage(m_nearestSampler, depthImage->viewHandle(), 2)
		.addImage(m_linearSampler, m_environmentImage->viewHandle(), 3)
		.addUniformBuffer(m_uniformBuffer.getBuffer(), m_uniformBuffer.getSize(), 4)
		.addUniformBuffer(m_lightUniformBuffer.getBuffer(), m_lightUniformBuffer.getSize(), 5)
		.addStorageImage(m_outputImage->viewHandle(), 6);
	m_descriptorSet = computeDescriptorSetBuilder.buildAndUpdate();

	return m_descriptorSet != VK_NULL_HANDLE;
}

void DeferredLightingPass::destroyDescriptorSet() {
	if (m_descriptorSet != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSet);
		m_descriptorSet = VK_NULL_HANDLE;
	}
}

void DeferredLightingPass::destroyCommandBuffer() {
	if (m_commandBuffer != VK_NULL_HANDLE) {
		m_device->getQueue(QueueType::eCompute)->freeCommandBuffer(m_commandBuffer);
		m_commandBuffer = VK_NULL_HANDLE;
	}
}

}