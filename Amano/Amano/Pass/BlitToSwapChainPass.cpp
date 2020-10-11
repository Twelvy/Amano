#include "BlitToSwapChainPass.h"
#include "../Builder/TransitionImageBarrierBuilder.h"

namespace Amano {

BlitToSwapChainPass::BlitToSwapChainPass(Device* device)
	: Pass(device, VK_PIPELINE_STAGE_TRANSFER_BIT)
	, m_commandBuffers()
{
}

BlitToSwapChainPass::~BlitToSwapChainPass() {
	cleanOnRenderTargetResized();
}

void BlitToSwapChainPass::cleanOnRenderTargetResized() {
	destroyCommandBuffers();
}

void BlitToSwapChainPass::recreateOnRenderTargetResized(uint32_t width, uint32_t height, Image* blitSourceImage) {
	recordCommands(width, height, blitSourceImage);
}

void BlitToSwapChainPass::recordCommands(uint32_t width, uint32_t height, Image* blitSourceImage) {
	destroyCommandBuffers();

	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	m_commandBuffers.resize(m_device->getSwapChainImages().size());

	for (size_t i = 0; i < m_device->getSwapChainImages().size(); ++i) {
		VkCommandBuffer blitCommandBuffer = pQueue->beginCommands();
		m_commandBuffers[i] = blitCommandBuffer;

		// transition the swapchain
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_device->getSwapChainImages()[i])
			.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.execute(blitCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = 0;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(blitCommandBuffer,
			//m_GBuffer.colorImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//m_GBuffer.normalImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//m_GBuffer.depthImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			blitSourceImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_device->getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		// transition the swapchain again for ui rendering
		transition
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
			.execute(blitCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		pQueue->endCommands(blitCommandBuffer);
	}
}

bool BlitToSwapChainPass::submit(uint32_t i, VkFence fence) {
	// submit blit
	// 1. wait for the semaphores
	// 2. signal the blit finished semaphore
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores.size());
	submitInfo.pWaitSemaphores = m_waitSemaphores.data();
	submitInfo.pWaitDstStageMask = m_waitPipelineStages.data();;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[i];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_signalSemaphore;

	auto pComputeQueue = m_device->getQueue(QueueType::eGraphics);
	if (!pComputeQueue->submit(&submitInfo, fence))
		return false;

	return true;
}


void BlitToSwapChainPass::destroyCommandBuffers() {
	auto pQueue = m_device->getQueue(QueueType::eGraphics);
	for (auto cmd : m_commandBuffers)
		pQueue->freeCommandBuffer(cmd);
	m_commandBuffers.clear();
}

}