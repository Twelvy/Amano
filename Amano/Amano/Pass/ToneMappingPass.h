#pragma once
#pragma once

#include "Pass.h"
#include "../Device.h"
#include "../Image.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"

namespace Amano {

// This class performs a simle tone mapping
// TEMPORARY: to work correctly, it expects the following image states:
//   -outputImage: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT
//   - colorImage: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
// After submitting, the images states are the same
class ToneMappingPass : public Pass {
public:
	ToneMappingPass(Device* device);
	~ToneMappingPass();

	Image* outputImage() const { return m_outputImage; }

	bool init();
	void recordCommands(uint32_t width, uint32_t height);

	void cleanOnRenderTargetResized();
	void recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* colorImage);

	bool submit();

private:
	void createOutputImage(uint32_t width, uint32_t height);
	void destroyOutputImage();
	bool createDescriptorSet(Image* colorImage);
	void destroyDescriptorSet();
	void destroyCommandBuffer();

private:
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkDescriptorSet m_descriptorSet;
	VkSampler m_nearestSampler;
	Image* m_outputImage;
	VkCommandBuffer m_commandBuffer;
};

}