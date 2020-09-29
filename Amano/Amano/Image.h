#pragma once

#include "Config.h"
#include "Device.h"

namespace Amano {

class Queue;

class Image
{
public:
	Image(Device* device);
	~Image();

	bool create(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	VkImageView createView(VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	void transitionLayout(Queue& queue, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

private:
	Device* m_device;
	VkFormat m_format;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
};

}
