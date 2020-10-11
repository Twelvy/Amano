#pragma once

#include "../Device.h"
#include "../Builder/RaytracingAccelerationStructureBuilder.h"
#include "../Builder/ShaderBindingTableBuilder.h"
#include "../Image.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"
#include "../glm.h"

#include <vector>

namespace Amano {

class RaytracingShadowPass {
public:
	RaytracingShadowPass(Device* device);
	~RaytracingShadowPass();

	VkSemaphore signalSemaphore() const { return m_signalSemaphore; }
	VkPipelineStageFlags pipelineStage() const { return m_pipelineStage; }

	bool init(std::vector<Mesh*>& meshes);
	void recordCommands(uint32_t width, uint32_t height, Image* finalImage, Image* colorImage);

	void addWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags pipelineStage);

	void onRenderTargetResized(uint32_t width, uint32_t height, Image* finalImage, Image* depthImage, Image* normalImage, Image* colorImage);
	
	void updateRayUniformBuffer(RayParams& ubo);
	void updateLightUniformBuffer(LightInformation& ubo);

	bool submit();

private:
	bool createDescriptorSet(Image* finalImage, Image* depthImage, Image* normalImage, Image* colorImage);
	void destroyDescriptorSet();
	void destroyCommandBuffer();

private:
	Device* m_device;
	//UniformBuffer<RayParams>* m_raytracingUniformBuffer;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkDescriptorSet m_descriptorSet;
	AccelerationStructures m_accelerationStructures;
	ShaderBindingTables m_shaderBindingTables;
	VkSampler m_nearestSampler;
	UniformBuffer<RayParams> m_rayUniformBuffer;
	UniformBuffer<LightInformation> m_lightUniformBuffer;

	VkCommandBuffer m_commandBuffer;
	VkSemaphore m_signalSemaphore;
	VkPipelineStageFlags m_pipelineStage;
	std::vector<VkSemaphore> m_waitSemaphores;
	std::vector<VkPipelineStageFlags> m_waitPipelineStages;
};

}
