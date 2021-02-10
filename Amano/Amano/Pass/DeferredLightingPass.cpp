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
	, m_uniformBuffer(device)
	, m_lightUniformBuffer(device)
	, m_materialUniformBuffer(device)
	, m_debugUniformBuffer(device)
	, m_outputImage{ nullptr }
	, m_ibl{}
	, m_commandBuffer{ VK_NULL_HANDLE }
	, m_cubemapDiffuseFiltering(device)
	, m_cubemapSpecularFiltering(device)
	, m_cubemapRotate(device)
	, m_iblLutPass(device)
{
}

DeferredLightingPass::~DeferredLightingPass() {
	cleanOnRenderTargetResized();

	vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
	vkDestroySampler(m_device->handle(), m_nearestSampler, nullptr);
	delete m_ibl.environmentImage;
	delete m_ibl.filteredDiffuseImage;
	delete m_ibl.filteredSpecularImage;
	delete m_ibl.lutImage;
}

bool DeferredLightingPass::init() {
	m_cubemapDiffuseFiltering.init();
	m_cubemapSpecularFiltering.init();
	m_cubemapRotate.init();
	m_iblLutPass.init();

	m_ibl.environmentImage = new Image(m_device);
	m_ibl.environmentImage->createCube("assets/textures/environments/ruins/ruinsEnvHDR.dds", *m_device->getQueue(QueueType::eGraphics));
	m_ibl.environmentImage->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	/*
	m_environmentImage->createCube(
		"assets/textures/rooitou_park_16k/environment/rooitou_posx.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_negx.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_posy.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_negy.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_posz.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_negz.hdr",
		*m_device->getQueue(QueueType::eGraphics),
		false);
	*/
	
	/*
	m_environmentImage->createCube(
		"assets/textures/environments/gym_entrance_posx.hdr",
		"assets/textures/environments/gym_entrance_negx.hdr",
		"assets/textures/environments/gym_entrance_posy.hdr",
		"assets/textures/environments/gym_entrance_negy.hdr",
		"assets/textures/environments/gym_entrance_posz.hdr",
		"assets/textures/environments/gym_entrance_negz.hdr",
		//"assets/textures/environments/gym/gym_entrance_posx.jpg",
		//"assets/textures/environments/gym/gym_entrance_negx.jpg",
		//"assets/textures/environments/gym/gym_entrance_posy.jpg",
		//"assets/textures/environments/gym/gym_entrance_negy.jpg",
		//"assets/textures/environments/gym/gym_entrance_posz.jpg",
		//"assets/textures/environments/gym/gym_entrance_negz.jpg",
		*m_device->getQueue(QueueType::eGraphics),
		false);
	*/
	// load the image then rotate it
	/*
	{
		Image temporaryImage(m_device);
		temporaryImage.createCube(
			"assets/textures/environments/gym_entrance_posx.hdr",
			"assets/textures/environments/gym_entrance_negx.hdr",
			"assets/textures/environments/gym_entrance_posy.hdr",
			"assets/textures/environments/gym_entrance_negy.hdr",
			"assets/textures/environments/gym_entrance_posz.hdr",
			"assets/textures/environments/gym_entrance_negz.hdr",
			*m_device->getQueue(QueueType::eGraphics),
			false);

		m_environmentImage = new Image(m_device);
		m_environmentImage->createCube(
			temporaryImage.getWidth(),
			temporaryImage.getHeight(),
			temporaryImage.getMipLevels(),
			temporaryImage.getFormat(),
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_environmentImage->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

		auto queue = m_device->getQueue(QueueType::eCompute);
		VkCommandBuffer commandBuffer = queue->beginSingleTimeCommands();
		
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_environmentImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_environmentImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
				.setAccessMasks(0, 0, VK_ACCESS_SHADER_WRITE_BIT)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, temporaryImage.handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, temporaryImage.getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setLayouts(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL)
				.setAccessMasks(0, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		// rotate everything
		m_cubemapRotate.setupAndRecord(commandBuffer, &temporaryImage, m_environmentImage);

		// transition final image to shader read
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_environmentImage->handle())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_environmentImage->getMipLevels())
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		queue->endSingleTimeCommands(commandBuffer);

		m_cubemapRotate.clean();
	}
	*/
	
	m_ibl.filteredDiffuseImage = new Image(m_device);
	m_ibl.filteredDiffuseImage->createCube(
		256, 256, 1,
		VK_FORMAT_R16G16B16A16_SFLOAT,  // TODO: VK_FORMAT_R16G16B16_SFLOAT
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_ibl.filteredDiffuseImage->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

	// filter diffuse
	{
		auto queue = m_device->getQueue(QueueType::eCompute);
		VkCommandBuffer commandBuffer = queue->beginSingleTimeCommands();

		// transition to storage
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_ibl.filteredDiffuseImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_ibl.filteredDiffuseImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
				.setAccessMasks(0, 0, VK_ACCESS_SHADER_WRITE_BIT)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		// filter!
		m_cubemapDiffuseFiltering.setupAndRecord(commandBuffer, m_ibl.environmentImage, m_ibl.filteredDiffuseImage);

		// transition final image to shader read
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_ibl.filteredDiffuseImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_ibl.filteredDiffuseImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		queue->endSingleTimeCommands(commandBuffer);

		m_cubemapDiffuseFiltering.clean();
	}

	//m_irradianceImage = new Image(m_device);
	/*
	m_irradianceImage->createCube(
		"assets/textures/rooitou_park_16k/irradiance/rooitou_park_irr_posx.hdr",
		"assets/textures/rooitou_park_16k/irradiance/rooitou_park_irr_negx.hdr",
		"assets/textures/rooitou_park_16k/irradiance/rooitou_park_irr_posy.hdr",
		"assets/textures/rooitou_park_16k/irradiance/rooitou_park_irr_negy.hdr",
		"assets/textures/rooitou_park_16k/irradiance/rooitou_park_irr_posz.hdr",
		"assets/textures/rooitou_park_16k/irradiance/rooitou_park_irr_negz.hdr",
		*m_device->getQueue(QueueType::eGraphics),
		false);
	*/
	//m_irradianceImage->createCube("assets/textures/rooitouDiffuseHDR.dds", *m_device->getQueue(QueueType::eGraphics));
	//m_irradianceImage->createCube("assets/textures/environments/ruins/ruinsDiffuseHDR.dds", *m_device->getQueue(QueueType::eGraphics));
	//m_irradianceImage->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

	uint32_t environmentSize = m_ibl.environmentImage->getWidth();
	m_ibl.filteredSpecularImage = new Image(m_device);
	m_ibl.filteredSpecularImage->createCube(
		environmentSize, environmentSize, m_ibl.environmentImage->getMipLevels(),
		VK_FORMAT_R16G16B16A16_SFLOAT,  // TODO: VK_FORMAT_R16G16B16_SFLOAT
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_ibl.filteredSpecularImage->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

	// filter specular
	{
		auto queue = m_device->getQueue(QueueType::eCompute);
		VkCommandBuffer commandBuffer = queue->beginSingleTimeCommands();

		// transition to storage
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_ibl.filteredSpecularImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_ibl.filteredSpecularImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
				.setAccessMasks(0, 0, VK_ACCESS_SHADER_WRITE_BIT)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		// filter!
		m_cubemapSpecularFiltering.setupAndRecord(commandBuffer, m_ibl.environmentImage, m_ibl.filteredSpecularImage);

		// transition final image to shader read
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_ibl.filteredSpecularImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_ibl.filteredSpecularImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 6)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		queue->endSingleTimeCommands(commandBuffer);

		m_cubemapSpecularFiltering.clean();
	}
	//m_specularIrradianceImage->createCube(
	//	"assets/textures/rooitou_park_16k/specular/rooitou_park_spec_posx.hdr",
	//	"assets/textures/rooitou_park_16k/specular/rooitou_park_spec_negx.hdr",
	//	"assets/textures/rooitou_park_16k/specular/rooitou_park_spec_posy.hdr",
	//	"assets/textures/rooitou_park_16k/specular/rooitou_park_spec_negy.hdr",
	//	"assets/textures/rooitou_park_16k/specular/rooitou_park_spec_posz.hdr",
	//	"assets/textures/rooitou_park_16k/specular/rooitou_park_spec_negz.hdr",
	//	*m_device->getQueue(QueueType::eGraphics),
	//	true);
	//m_specularIrradianceImage->createCube("assets/textures/rooitouSpecularHDR.dds", *m_device->getQueue(QueueType::eGraphics));
	//m_ibl.filteredSpecularImage->createCube("assets/textures/environments/ruins/ruinsSpecularHDR.dds", *m_device->getQueue(QueueType::eGraphics));
	/*
	m_specularIrradianceImage->createCube(
		"assets/textures/rooitou_park_16k/environment/rooitou_posx.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_negx.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_posy.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_negy.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_posz.hdr",
		"assets/textures/rooitou_park_16k/environment/rooitou_negz.hdr",
		*m_device->getQueue(QueueType::eGraphics),
		true);
	*/

	m_ibl.lutImage = new Image(m_device);
	//m_ibl.lutImage->create2D("assets/textures/ibl_brdf_lut.png", *m_device->getQueue(QueueType::eGraphics), false);
	//m_ibl.lutImage->create2D("assets/textures/rooitouBrdf.dds", *m_device->getQueue(QueueType::eGraphics));
	//m_ibl.lutImage->create2D("assets/textures/environments/ruins/ruinsBrdf.dds", *m_device->getQueue(QueueType::eGraphics));
	m_ibl.lutImage->create2D(512, 512, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_ibl.lutImage->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR);

	// create lut
	{
		auto queue = m_device->getQueue(QueueType::eCompute);
		VkCommandBuffer commandBuffer = queue->beginSingleTimeCommands();

		// transition to storage
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_ibl.lutImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_ibl.lutImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 1)
				.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)
				.setAccessMasks(0, 0, VK_ACCESS_SHADER_WRITE_BIT)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		// generate
		m_iblLutPass.setupAndRecord(commandBuffer, m_ibl.lutImage);

		// transition final image to shader read
		{
			TransitionImageBarrierBuilder<1> transition;
			transition
				.setImage(0, m_ibl.lutImage->handle())
				.setBaseMipLevel(0, 0)
				.setLevelCount(0, m_ibl.lutImage->getMipLevels())
				.setBaseLayer(0, 0)
				.setLayerCount(0, 1)
				.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
				.setLayouts(0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.setAccessMasks(0, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
				.execute(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		queue->endSingleTimeCommands(commandBuffer);

		m_iblLutPass.clean();
	}
	

	DescriptorSetLayoutBuilder computeDescriptorSetLayoutbuilder;
	computeDescriptorSetLayoutbuilder
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // albedo image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // normal image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // depth image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // environment image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)            // camera information
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)            // light information
		.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)             // output image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // irradiance image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // specular irradiance image
		.addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)    // IBL LUT image
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)            // material information
		.addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);           // debug information
	m_descriptorSetLayout = computeDescriptorSetLayoutbuilder.build(*m_device);

	// create raytracing pipeline layout
	PipelineLayoutBuilder computePipelineLayoutBuilder;
	computePipelineLayoutBuilder.addDescriptorSetLayout(m_descriptorSetLayout);
	m_pipelineLayout = computePipelineLayoutBuilder.build(*m_device);

	// load the shaders and create the pipeline
	ComputePipelineBuilder computePipelineBuilder(m_device);
	computePipelineBuilder
		.addShader("compiled_shaders/deferred_lighting.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	m_pipeline = computePipelineBuilder.build(m_pipelineLayout);

	// create a sampler for the textures
	SamplerBuilder nearestSamplerBuilder;
	nearestSamplerBuilder
		.setMaxLod(0)
		.setFilter(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
	m_nearestSampler = nearestSamplerBuilder.build(*m_device);

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

void DeferredLightingPass::updateMaterialUniformBuffer(MaterialInformation& ubo) {
	m_materialUniformBuffer.update(ubo);
}

void DeferredLightingPass::updateDebugUniformBuffer(DebugInformation& ubo) {
	m_debugUniformBuffer.update(ubo);
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
	submitInfo.pWaitDstStageMask = m_waitPipelineStages.data();

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
		VK_FORMAT_R32G32B32A32_SFLOAT, //formats.colorFormat,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
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
		.addImage(m_ibl.environmentImage->sampler(), m_ibl.environmentImage->viewHandle(), 3)
		.addUniformBuffer(m_uniformBuffer.getBuffer(), m_uniformBuffer.getSize(), 4)
		.addUniformBuffer(m_lightUniformBuffer.getBuffer(), m_lightUniformBuffer.getSize(), 5)
		.addStorageImage(m_outputImage->viewHandle(), 6)
		.addImage(m_ibl.filteredDiffuseImage->sampler(), m_ibl.filteredDiffuseImage->viewHandle(), 7)
		.addImage(m_ibl.filteredSpecularImage->sampler(), m_ibl.filteredSpecularImage->viewHandle(), 8)
		.addImage(m_ibl.lutImage->sampler(), m_ibl.lutImage->viewHandle(), 9)
		.addUniformBuffer(m_materialUniformBuffer.getBuffer(), m_materialUniformBuffer.getSize(), 10)
		.addUniformBuffer(m_debugUniformBuffer.getBuffer(), m_debugUniformBuffer.getSize(), 11);
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