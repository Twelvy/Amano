#pragma once

#include "Pass.h"
#include "../Device.h"
#include "../Builder/RaytracingAccelerationStructureBuilder.h"
#include "../Builder/ShaderBindingTableBuilder.h"
#include "../Image.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"

namespace Amano {

// This class raytraces the scene to generate some shadows
// TEMPORARY: to work correctly, the pass expect the following states for the images
//   - finalImage: VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_TRANSFER_READ_BIT
//   - depthImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
//   - normalImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
//   - colorImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
// After submitting, the images states are:
//   - finalImage: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT
//   - colorImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT
//   - the other images are unchanged
class RaytracingShadowPass : public Pass {
public:
	RaytracingShadowPass(Device* device);
	~RaytracingShadowPass();

	bool init(std::vector<Mesh*>& meshes);

	void recordCommands(uint32_t width, uint32_t height, Image* finalImage, Image* colorImage);

	void cleanOnRenderTargetResized();
	void recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* finalImage, Image* depthImage, Image* normalImage, Image* colorImage);
	
	void updateRayUniformBuffer(RayParams& ubo);
	void updateLightUniformBuffer(LightInformation& ubo);

	bool submit() override final;

private:
	bool createDescriptorSet(Image* finalImage, Image* depthImage, Image* normalImage, Image* colorImage);
	void destroyDescriptorSet();
	void destroyCommandBuffer();

private:
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
};

}
