#pragma once

#include "Pass.h"
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
//   - outputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT
// After submitting, the images states are:
//   - outputImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
//   - the others are unchanged
// Everything is ready to be sampled by the next passes
class DeferredLightingPass : public Pass {
public:
	DeferredLightingPass(Device* device);
	~DeferredLightingPass();

	Image* outputImage() const { return m_outputImage; }

	bool init();
	void recordCommands(uint32_t width, uint32_t height);

	void onRenderTargetResized(uint32_t width, uint32_t height, Image* albedoImage, Image* normalImage, Image* depthImage);

	void updateUniformBuffer(RayParams& ubo);
	void updateLightUniformBuffer(LightInformation& ubo);

	bool submit() override final;

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
	VkSampler m_linearSampler;
	UniformBuffer<RayParams> m_uniformBuffer;
	UniformBuffer<LightInformation> m_lightUniformBuffer;
	Image* m_outputImage;
	Image* m_environmentImage;
	VkCommandBuffer m_commandBuffer;
};

}