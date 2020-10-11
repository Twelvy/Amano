#pragma once

#include "Pass.h"
#include "../Device.h"
#include "../Image.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"

namespace Amano {

// This class performs the blit from an image to the swapchain
// TEMPORARY: to work correctly, it expects the following image states:
//   - source image:  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT
// After submitting, the images states are the same
class BlitToSwapChainPass : public Pass {
public:
	BlitToSwapChainPass(Device* device);
	~BlitToSwapChainPass();

	void recordCommands(uint32_t width, uint32_t height, Image* blitSourceImage);

	void cleanOnRenderTargetResized();
	void recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* blitSourceImage);

	bool submit(uint32_t i, VkFence fence);

private:
	void destroyCommandBuffers();

private:
	std::vector<VkCommandBuffer> m_commandBuffers;
};

}