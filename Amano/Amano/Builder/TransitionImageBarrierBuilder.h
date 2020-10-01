#pragma once

#include "../Config.h"

namespace Amano {

// Helper class to transition an image layout
// This can be used several times to chain transitions
// Some options are missing and will be added in the future based on the necessity
// By default:
//   - oldLayout and newLayou are VK_IMAGE_LAYOUT_UNDEFINED
//   - srcAccessMask and dstAccessMask are 0
//   - aspectMask is VK_IMAGE_ASPECT_COLOR_BIT
//   - layerCount is 1
//   - baseMipLevel is 0
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
			m_barriers[i].srcAccessMask = 0;
			m_barriers[i].dstAccessMask = 0;
		}
	}

	uint32_t getCount() { return COUNT; }

	TransitionImageBarrierBuilder& setImage(uint32_t index, VkImage image) {
		m_barriers[index].image = image;
		return *this;
	}

	TransitionImageBarrierBuilder& setLevelCount(uint32_t index, uint32_t levelCount) {
		m_barriers[index].subresourceRange.levelCount = levelCount;
		return *this;
	}

	TransitionImageBarrierBuilder& setBaseMipLevel(uint32_t index, uint32_t mipLevel) {
		m_barriers[index].subresourceRange.baseMipLevel = mipLevel;
		return *this;
	}

	TransitionImageBarrierBuilder& setAspectMask(uint32_t index, VkImageAspectFlags aspectMask) {
		m_barriers[index].subresourceRange.aspectMask = aspectMask;
		return *this;
	}

	// NOTE: maybe those barriers should be tied to the image.
	// They can store the actual layout of it
	// However, in a multithread environment, it doesn't work well...
	TransitionImageBarrierBuilder& setLayouts(uint32_t index, VkImageLayout oldLayout, VkImageLayout newLayout) {
		m_barriers[index].oldLayout = oldLayout;
		m_barriers[index].newLayout = newLayout;
		return *this;
	}

	// NOTE: maybe those barriers should be tied to the image.
	// They can store the actual layout of it
	// However, in a multithread environment, it doesn't work well...
	TransitionImageBarrierBuilder& setAccessMasks(uint32_t index, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
		m_barriers[index].srcAccessMask = srcAccessMask;
		m_barriers[index].dstAccessMask = dstAccessMask;
		return *this;
	}

	void execute(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
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