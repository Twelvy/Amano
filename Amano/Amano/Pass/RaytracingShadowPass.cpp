#include "RaytracingShadowPass.h"
#include "../Builder/DescriptorSetBuilder.h"
#include "../Builder/DescriptorSetLayoutBuilder.h"
#include "../Builder/PipelineLayoutBuilder.h"
#include "../Builder/RaytracingPipelineBuilder.h"
#include "../Builder/SamplerBuilder.h"
#include "../Builder/TransitionImageBarrierBuilder.h"

#include <iostream>

namespace Amano {

RaytracingShadowPass::RaytracingShadowPass(Device* device)
	: Pass(device, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV)
	, m_descriptorSetLayout{ VK_NULL_HANDLE }
	, m_pipelineLayout{ VK_NULL_HANDLE }
	, m_pipeline{ VK_NULL_HANDLE }
	, m_descriptorSet{ VK_NULL_HANDLE }
	, m_accelerationStructures()
	, m_shaderBindingTables()
	, m_nearestSampler{ VK_NULL_HANDLE }
	, m_rayUniformBuffer(device)
	, m_lightUniformBuffer(device)
	, m_commandBuffer{ VK_NULL_HANDLE }
{
}

RaytracingShadowPass::~RaytracingShadowPass() {
	destroyDescriptorSet();
	destroyCommandBuffer();

	m_accelerationStructures.clean(m_device);
	m_shaderBindingTables.clean(m_device);
	vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
	vkDestroySampler(m_device->handle(), m_nearestSampler, nullptr);
}

bool RaytracingShadowPass::init(std::vector<Mesh*>& meshes) {
	// create layout for the raytracing pipeline
	DescriptorSetLayoutBuilder raytracingDescriptorSetLayoutbuilder;
	raytracingDescriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, VK_SHADER_STAGE_RAYGEN_BIT_NV)  // acceleration structure
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV)              // output image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV)             // ray parameters
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_NV)     // depth image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_NV)     // normal image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_NV)     // albedo image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);            // light information
	m_descriptorSetLayout = raytracingDescriptorSetLayoutbuilder.build(*m_device);

	// create raytracing pipeline layout
	PipelineLayoutBuilder raytracingPipelineLayoutBuilder;
	raytracingPipelineLayoutBuilder.addDescriptorSetLayout(m_descriptorSetLayout);
	m_pipelineLayout = raytracingPipelineLayoutBuilder.build(*m_device);

	// load the shaders and create the pipeline
	RaytracingPipelineBuilder raytracingPipelineBuilder(m_device);
	raytracingPipelineBuilder
		.addShader("compiled_shaders/shadow_raygen.spv", VK_SHADER_STAGE_RAYGEN_BIT_NV)
		.addShader("compiled_shaders/shadow_miss.spv", VK_SHADER_STAGE_MISS_BIT_NV)
		.addShader("compiled_shaders/shadow_closesthit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
	m_pipeline = raytracingPipelineBuilder.build(m_pipelineLayout, 1);

	// create the shader binding table associated to this pipeline
	ShaderBindingTableBuilder sbtBuilder(m_device, m_pipeline);
	sbtBuilder
		.addShader(ShaderBindingTableBuilder::Stage::eRayGen, 0)
		.addShader(ShaderBindingTableBuilder::Stage::eMiss, 1)
		.addShader(ShaderBindingTableBuilder::Stage::eClosestHit, 2);
	m_shaderBindingTables = sbtBuilder.build();

	// build the acceleration structure for 1 model
	// we should extend it to more models
	RaytracingAccelerationStructureBuilder accelerationStructureBuilder(m_device, m_pipeline);
	for (auto mesh : meshes)
		accelerationStructureBuilder.addGeometry(*mesh);
	m_accelerationStructures = accelerationStructureBuilder.build();

	// create a sampler for the depth texture
	SamplerBuilder samplerBuilder;
	samplerBuilder
		.setMaxLoad(1)
		.setFilter(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
	m_nearestSampler = samplerBuilder.build(*m_device);

	return true;
}

bool RaytracingShadowPass::createDescriptorSet(Image* finalImage, Image* depthImage, Image* normalImage, Image* colorImage) {
	// update the descriptor set for raytracing
	DescriptorSetBuilder raytracingDescriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
	raytracingDescriptorSetBuilder
		.addAccelerationStructure(&m_accelerationStructures.top.handle, 0)
		.addStorageImage(finalImage->viewHandle(), 1)
		.addUniformBuffer(m_rayUniformBuffer.getBuffer(), m_rayUniformBuffer.getSize(), 2)
		.addImage(m_nearestSampler, depthImage->viewHandle(), 3)
		.addImage(m_nearestSampler, normalImage->viewHandle(), 4)
		.addImage(m_nearestSampler, colorImage->viewHandle(), 5)
		.addUniformBuffer(m_lightUniformBuffer.getBuffer(), m_lightUniformBuffer.getSize(), 6);
	m_descriptorSet = raytracingDescriptorSetBuilder.buildAndUpdate();

	return m_descriptorSet != VK_NULL_HANDLE;
}

void RaytracingShadowPass::recordCommands(uint32_t width, uint32_t height, Image* finalImage, Image* colorImage) {
	destroyCommandBuffer();

	Queue* pQueue = m_device->getQueue(QueueType::eGraphics);
	m_commandBuffer = pQueue->beginCommands();

	// transition the raytracing output buffer from copy to storage
	TransitionImageBarrierBuilder<1> transition;
	transition
		.setImage(0, finalImage->handle())
		.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
		.setAccessMasks(0, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT)
		.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
		.execute(m_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipeline);
	vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	m_device->getExtensions().vkCmdTraceRaysNV(m_commandBuffer,
		m_shaderBindingTables.rgenShaderBindingTable.buffer, 0, // offset in the shader binding table for rgen
		m_shaderBindingTables.missShaderBindingTable.buffer, 0, m_shaderBindingTables.missShaderBindingTable.groupSize, // offset and stride for miss
		m_shaderBindingTables.chitShaderBindingTable.buffer, 0, m_shaderBindingTables.chitShaderBindingTable.groupSize, // offset and stride for chit
		nullptr, 0, 0,
		width, height, 1);

	// transition the raytracing image to be blitted
	transition
		.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT)
		.execute(m_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	// transition the lighting image for storage access
	TransitionImageBarrierBuilder<1> gbufferTransition;
	gbufferTransition
		.setImage(0, colorImage->handle())
		.setLayouts(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL)
		.setAccessMasks(0, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT)
		.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
		.execute(m_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	pQueue->endCommands(m_commandBuffer);
}

void RaytracingShadowPass::cleanOnRenderTargetResized() {
	destroyDescriptorSet();
	destroyCommandBuffer();
}

void RaytracingShadowPass::recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* finalImage, Image* depthImage, Image* normalImage, Image* colorImage) {
	createDescriptorSet(finalImage, depthImage, normalImage, colorImage);
	recordCommands(width, height, finalImage, colorImage);
}

void RaytracingShadowPass::updateRayUniformBuffer(RayParams& ubo) {
	m_rayUniformBuffer.update(ubo);
}

void RaytracingShadowPass::updateLightUniformBuffer(LightInformation& ubo) {
	m_lightUniformBuffer.update(ubo);
}

bool RaytracingShadowPass::submit() {
	if (m_commandBuffer == VK_NULL_HANDLE)
		return false;

	// submit raytracing
	// 1. wait for the semaphores
	// 2. signal the pass semaphore
	VkSubmitInfo raytracingSubmitInfo{};
	raytracingSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	raytracingSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores.size());
	raytracingSubmitInfo.pWaitSemaphores = m_waitSemaphores.data();
	raytracingSubmitInfo.pWaitDstStageMask = m_waitPipelineStages.data();

	raytracingSubmitInfo.commandBufferCount = 1;
	raytracingSubmitInfo.pCommandBuffers = &m_commandBuffer;

	raytracingSubmitInfo.signalSemaphoreCount = 1;
	raytracingSubmitInfo.pSignalSemaphores = &m_signalSemaphore;

	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	if (!pQueue->submit(&raytracingSubmitInfo, VK_NULL_HANDLE))
		return false;

	return true;
}

void RaytracingShadowPass::destroyDescriptorSet() {
	if (m_descriptorSet != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSet);
		m_descriptorSet = VK_NULL_HANDLE;
	}
}

void RaytracingShadowPass::destroyCommandBuffer() {
	if (m_commandBuffer != VK_NULL_HANDLE) {
		m_device->getQueue(QueueType::eGraphics)->freeCommandBuffer(m_commandBuffer);
		m_commandBuffer = VK_NULL_HANDLE;
	}
}

}
