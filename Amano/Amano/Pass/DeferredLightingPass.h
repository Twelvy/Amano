#pragma once

#include "Pass.h"
#include "CubemapRotatePass.h"
#include "CubemapFilteringPass.h"
#include "CubemapSpecularFilteringPass.h"
#include "IBLLutPass.h"
#include "../Device.h"
#include "../Image.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"

namespace Amano {

// This class performs the lighting with a compute shader
// TEMPORARY: to work correctly, it expects the following image states:
//   - albedoImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
//   - normalImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
//   - depthImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
//   - outputImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
// After submitting, the images states are the same
// Everything is ready to be sampled by the next passes
class DeferredLightingPass : public Pass {
public:
	DeferredLightingPass(Device* device);
	~DeferredLightingPass();

	Image* outputImage() const { return m_outputImage; }

	bool init();
	void recordCommands(uint32_t width, uint32_t height);

	void cleanOnRenderTargetResized();
	void recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* albedoImage, Image* normalImage, Image* depthImage);

	void updateUniformBuffer(RayParams& ubo);
	void updateLightUniformBuffer(LightInformation& ubo);
	void updateMaterialUniformBuffer(MaterialInformation& ubo);
	void updateDebugUniformBuffer(DebugInformation& ubo);

	bool submit();

private:
	void createOutputImage(uint32_t width, uint32_t height);
	void destroyOutputImage();
	bool createDescriptorSet(Image* albedoImage, Image* normalImage, Image* depthImage);
	void destroyDescriptorSet();
	void destroyCommandBuffer();

private:
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkDescriptorSet m_descriptorSet;
	VkSampler m_nearestSampler;
	UniformBuffer<RayParams> m_uniformBuffer;
	UniformBuffer<LightInformation> m_lightUniformBuffer;
	UniformBuffer<MaterialInformation> m_materialUniformBuffer;
	UniformBuffer<DebugInformation> m_debugUniformBuffer;
	Image* m_outputImage;
	struct {
		Image* environmentImage = nullptr;
		Image* filteredDiffuseImage = nullptr;
		Image* filteredSpecularImage = nullptr;
		Image* lutImage = nullptr;
	} m_ibl;
	VkCommandBuffer m_commandBuffer;

	CubemapDiffuseFilteringPass m_cubemapDiffuseFiltering;
	CubemapSpecularFilteringPass m_cubemapSpecularFiltering;
	CubemapRotatePass m_cubemapRotate;
	IBLLutPass m_iblLutPass;
};

}