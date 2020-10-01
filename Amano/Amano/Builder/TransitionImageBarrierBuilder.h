#pragma once

#include "../Config.h"

namespace Amano {

template<uint32_t COUNT>
class TransitionImageBarrierBuilder {

public:
	TransitionImageBarrierBuilder()
		: m_barriers()
	{
		for (int i = 0; i < m_barriers.size(); ++i) {
			m_barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			m_barriers[i].pNext = nullptr;
			m_barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			m_barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			m_barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			m_barriers[i].subresourceRange.baseArrayLayer = 0;
			m_barriers[i].subresourceRange.layerCount = 1;
			m_barriers[i].subresourceRange.levelCount = 1;
			m_barriers[i].subresourceRange.baseMipLevel = 0;
			m_barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			m_barriers[i].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			m_barriers[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			m_barriers[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
	}

	TransitionImageBarrierBuilder& setLayoutTransition(uint32_t index, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
		m_barriers[index].image = image;
		m_barriers[index].oldLayout = oldLayout;
		m_barriers[index].newLayout = newLayout;
		m_barriers[index].srcAccessMask = srcAccessMask;
		m_barriers[index].dstAccessMask = dstAccessMask;

		return *this;
	}

	void execute(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits srcStageMask, VkPipelineStageFlagBits dstStageMask) {
		vkCmdPipelineBarrier(commandBuffer,
			srcStageMask, dstStageMask, 0,
			0, nullptr,
			0, nullptr,
			COUNT,
			m_barriers.data());
	}

private:
	std::array<VkImageMemoryBarrier, COUNT> m_barriers;
};

}