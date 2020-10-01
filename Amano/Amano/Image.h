#pragma once

#include "Config.h"
#include "Device.h"

#include <string>

namespace Amano {

class Queue;

// This class is used to create a texture and its associated buffer
// It is also used to generate a view on it
class Image
{
public:
	Image(Device* device);
	~Image();

	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }
	uint32_t getMipLevels() const { return m_mipLevels; }
	VkImage handle() { return m_image; }

	bool create(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	bool create(const std::string& filename, Queue& queue);
	VkImageView createView(VkImageAspectFlags aspectFlags);

	void transitionLayout(Queue& queue, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	void copyBufferToImage(Queue& queue, VkBuffer buffer);
	void generateMipmaps(Queue& queue);
private:
	Device* m_device;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mipLevels;
	VkFormat m_format;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
};

}
