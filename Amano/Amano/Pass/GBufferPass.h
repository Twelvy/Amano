#pragma once

#include "Pass.h"
#include "../Device.h"
#include "../Image.h"
#include "../Mesh.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"

namespace Amano {

// This class generates the GBuffer
// TEMPORARY:
// After submitting, the images states are  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
class GBufferPass : public Pass {
public:
	GBufferPass(Device* device);
	~GBufferPass();

	Image* albedoImage() const { return m_albedoImage; }
	Image* normalImage() const { return m_normalImage; }
	Image* depthImage()  const { return m_depthImage;  }

	bool init();

	void recordCommands(uint32_t width, uint32_t height, const Mesh* mesh);

	void cleanOnRenderTargetResized();
	void recreateOnRenderTargetResized(uint32_t width, uint32_t height, const Mesh* mesh, Image* texture);
	
	void updateUniformBuffer(PerFrameUniformBufferObject& ubo);

	bool submit() override final;

private:
	void createGBufferImages(uint32_t width, uint32_t height);
	void destroyGBufferImages();
	bool createDescriptorSet(Image* texture);
	void destroyDescriptorSet();
	void destroyCommandBuffer();

	struct Formats {
		VkFormat depthFormat;
		VkFormat colorFormat;
		VkFormat normalFormat;
	};
	Formats getFormats();

private:
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	VkRenderPass m_renderPass;
	VkDescriptorSet m_descriptorSet;
	VkFramebuffer m_framebuffer;
	UniformBuffer<PerFrameUniformBufferObject> m_uniformBuffer;
	Image* m_albedoImage;
	Image* m_normalImage;
	Image* m_depthImage;

	VkCommandBuffer m_commandBuffer;
};

}
