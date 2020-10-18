#pragma once

#include "Device.h"

#include <vulkan/vulkan.h>
#include <string>

namespace Amano {

class Queue;

// This class is used to create a texture and its associated buffer
// It is also used to generate a view on it
class Image
{
private:
	enum class Type {
		eUnknown,
		eTexture2D,
		eTextureCube
	};

public:
	Image(Device* device);
	~Image();

	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }
	uint32_t getMipLevels() const { return m_mipLevels; }
	VkImage handle() const { return m_image; }
	VkImageView viewHandle() const { return m_imageView; }
	VkSampler sampler() const { return m_imageSampler; }

	bool create2D(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	bool create2D(const std::string& filename, Queue& queue, bool generateMips);
	// only loads DDS files
	bool create2D(const std::string& filename, Queue& queue);

	bool createCube(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	bool createCube(
		const std::string& filenamePosX,
		const std::string& filenameNegX,
		const std::string& filenamePosY,
		const std::string& filenameNegY,
		const std::string& filenamePosZ,
		const std::string& filenameNegZ,
		Queue& queue,
		bool generateMips);

	// only loads DDS files with RGBA32f formats inside
	bool createCube(const std::string& filename, Queue& queue);

	bool createView(VkImageAspectFlags aspectFlags);
	bool createSampler(VkFilter magFilter, VkFilter minFilter);

	void transitionLayout(Queue& queue, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	void transitionLayoutInternal(Queue& queue, uint32_t layer, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(Queue& queue, VkBuffer buffer, uint32_t layer);
	void generateMipmaps(Queue& queue, uint32_t layer);
private:
	Device* m_device;
	Type m_type;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mipLevels;
	VkFormat m_format;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
	VkImageView m_imageView;
	VkSampler m_imageSampler;
};

}
