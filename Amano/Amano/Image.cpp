#include "Image.h"
#include "Queue.h"

#include "Builder/SamplerBuilder.h"
#include "Builder/TransitionImageBarrierBuilder.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

namespace {

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool isDepthFormat(VkFormat format) {
	return format == VK_FORMAT_D16_UNORM
		|| format == VK_FORMAT_X8_D24_UNORM_PACK32
		|| format == VK_FORMAT_D32_SFLOAT
		|| format == VK_FORMAT_D16_UNORM_S8_UINT
		|| format == VK_FORMAT_D24_UNORM_S8_UINT
		|| format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}


VkImageAspectFlags getAspect(VkFormat format) {
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	if (isDepthFormat(format)) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
			aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else {
		aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return aspect;
}

size_t formatPixelSize(VkFormat format) {
	switch (format)
	{
	case VK_FORMAT_UNDEFINED:
		return 0;
	case VK_FORMAT_R4G4_UNORM_PACK8:
		return 1;
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
	case VK_FORMAT_R5G6B5_UNORM_PACK16:
	case VK_FORMAT_B5G6R5_UNORM_PACK16:
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		return 2;
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_SNORM:
	case VK_FORMAT_R8_USCALED:
	case VK_FORMAT_R8_SSCALED:
	case VK_FORMAT_R8_UINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_SRGB:
		return 1;
	case VK_FORMAT_R8G8_UNORM:
	case VK_FORMAT_R8G8_SNORM:
	case VK_FORMAT_R8G8_USCALED:
	case VK_FORMAT_R8G8_SSCALED:
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R8G8_SINT:
	case VK_FORMAT_R8G8_SRGB:
		return 2;
	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SNORM:
	case VK_FORMAT_R8G8B8_USCALED:
	case VK_FORMAT_R8G8B8_SSCALED:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R8G8B8_SINT:
	case VK_FORMAT_R8G8B8_SRGB:
	case VK_FORMAT_B8G8R8_UNORM:
	case VK_FORMAT_B8G8R8_SNORM:
	case VK_FORMAT_B8G8R8_USCALED:
	case VK_FORMAT_B8G8R8_SSCALED:
	case VK_FORMAT_B8G8R8_UINT:
	case VK_FORMAT_B8G8R8_SINT:
	case VK_FORMAT_B8G8R8_SRGB:
		return 3;
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SNORM:
	case VK_FORMAT_R8G8B8A8_USCALED:
	case VK_FORMAT_R8G8B8A8_SSCALED:
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R8G8B8A8_SINT:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SNORM:
	case VK_FORMAT_B8G8R8A8_USCALED:
	case VK_FORMAT_B8G8R8A8_SSCALED:
	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_B8G8R8A8_SINT:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
	case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_UINT_PACK32:
	case VK_FORMAT_A2R10G10B10_SINT_PACK32:
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:
	case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		return 4;
	case VK_FORMAT_R16_UNORM:
	case VK_FORMAT_R16_SNORM:
	case VK_FORMAT_R16_USCALED:
	case VK_FORMAT_R16_SSCALED:
	case VK_FORMAT_R16_UINT:
	case VK_FORMAT_R16_SINT:
	case VK_FORMAT_R16_SFLOAT:
		return 2;
	case VK_FORMAT_R16G16_UNORM:
	case VK_FORMAT_R16G16_SNORM:
	case VK_FORMAT_R16G16_USCALED:
	case VK_FORMAT_R16G16_SSCALED:
	case VK_FORMAT_R16G16_UINT:
	case VK_FORMAT_R16G16_SINT:
	case VK_FORMAT_R16G16_SFLOAT:
		return 4;
	case VK_FORMAT_R16G16B16_UNORM:
	case VK_FORMAT_R16G16B16_SNORM:
	case VK_FORMAT_R16G16B16_USCALED:
	case VK_FORMAT_R16G16B16_SSCALED:
	case VK_FORMAT_R16G16B16_UINT:
	case VK_FORMAT_R16G16B16_SINT:
	case VK_FORMAT_R16G16B16_SFLOAT:
		return 6;
	case VK_FORMAT_R16G16B16A16_UNORM:
	case VK_FORMAT_R16G16B16A16_SNORM:
	case VK_FORMAT_R16G16B16A16_USCALED:
	case VK_FORMAT_R16G16B16A16_SSCALED:
	case VK_FORMAT_R16G16B16A16_UINT:
	case VK_FORMAT_R16G16B16A16_SINT:
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 8;
	case VK_FORMAT_R32_UINT:
	case VK_FORMAT_R32_SINT:
	case VK_FORMAT_R32_SFLOAT:
		return 4;
	case VK_FORMAT_R32G32_UINT:
	case VK_FORMAT_R32G32_SINT:
	case VK_FORMAT_R32G32_SFLOAT:
		return 8;
	case VK_FORMAT_R32G32B32_UINT:
	case VK_FORMAT_R32G32B32_SINT:
	case VK_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case VK_FORMAT_R32G32B32A32_UINT:
	case VK_FORMAT_R32G32B32A32_SINT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	case VK_FORMAT_R64_UINT:
	case VK_FORMAT_R64_SINT:
	case VK_FORMAT_R64_SFLOAT:
		return 8;
	case VK_FORMAT_R64G64_UINT:
	case VK_FORMAT_R64G64_SINT:
	case VK_FORMAT_R64G64_SFLOAT:
		return 16;
	case VK_FORMAT_R64G64B64_UINT:
	case VK_FORMAT_R64G64B64_SINT:
	case VK_FORMAT_R64G64B64_SFLOAT:
		return 24;
	case VK_FORMAT_R64G64B64A64_UINT:
	case VK_FORMAT_R64G64B64A64_SINT:
	case VK_FORMAT_R64G64B64A64_SFLOAT:
		return 32;
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		return 4;
	case VK_FORMAT_D16_UNORM:
		return 2;
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
		return 4;
	case VK_FORMAT_S8_UINT:
		return 1;
	case VK_FORMAT_D16_UNORM_S8_UINT:
		return 3;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		return 4;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return 5;
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case VK_FORMAT_BC2_UNORM_BLOCK:
	case VK_FORMAT_BC2_SRGB_BLOCK:
	case VK_FORMAT_BC3_UNORM_BLOCK:
	case VK_FORMAT_BC3_SRGB_BLOCK:
	case VK_FORMAT_BC4_UNORM_BLOCK:
	case VK_FORMAT_BC4_SNORM_BLOCK:
	case VK_FORMAT_BC5_UNORM_BLOCK:
	case VK_FORMAT_BC5_SNORM_BLOCK:
	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
	case VK_FORMAT_BC7_UNORM_BLOCK:
	case VK_FORMAT_BC7_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	case VK_FORMAT_EAC_R11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11_SNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case VK_FORMAT_G8B8G8R8_422_UNORM:
	case VK_FORMAT_B8G8R8G8_422_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
	case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
	case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
	case VK_FORMAT_R10X6_UNORM_PACK16:
	case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
	case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
	case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
	case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_R12X4_UNORM_PACK16:
	case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
	case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
	case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
	case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_G16B16G16R16_422_UNORM:
	case VK_FORMAT_B16G16R16G16_422_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
	case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
	case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT:
	case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT:
	default:
		return 0;  // not supported for now
	}
}

// temporary
typedef unsigned long       DWORD;
typedef unsigned int        UINT;

struct DDS_PIXELFORMAT {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
};

typedef struct {
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;
} DDS_HEADER;

}

namespace Amano {

Image::Image(Device* device)
	: m_device{ device }
	, m_type{ Type::eUnknown }
	, m_width{ 0 }
	, m_height{ 0 }
	, m_mipLevels{ 0 }
	, m_format{ VK_FORMAT_UNDEFINED }
	, m_image{ VK_NULL_HANDLE }
	, m_imageMemory{ VK_NULL_HANDLE }
	, m_imageView{ VK_NULL_HANDLE }
	, m_imageSampler{ VK_NULL_HANDLE }
{
}

Image::~Image() {
	m_device->freeDeviceMemory(m_imageMemory);
	vkDestroyImageView(m_device->handle(), m_imageView, nullptr);
	vkDestroyImage(m_device->handle(), m_image, nullptr);
	vkDestroySampler(m_device->handle(), m_imageSampler, nullptr);
}

bool Image::create2D(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
	m_type = Type::eTexture2D;
	m_width = width;
	m_height = height;
	m_mipLevels = mipLevels;
	m_format = format;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = m_width;
	imageInfo.extent.height = m_height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = m_mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = m_format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(m_device->handle(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
		std::cerr << "failed to create image!" << std::endl;
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device->handle(), m_image, &memRequirements);

	m_imageMemory = m_device->allocateMemory(memRequirements, properties);
	if (m_imageMemory == VK_NULL_HANDLE)
		return false;

	vkBindImageMemory(m_device->handle(), m_image, m_imageMemory, 0);

	return createView(getAspect(m_format));
}

bool Image::create2D(const std::string& filename, Queue& queue, bool generateMips) {
	m_type = Type::eTexture2D;
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		std::cerr << "failed to load texture image!" << std::endl;
		return false;
	}

	m_mipLevels = generateMips ? static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;
	m_width = static_cast<uint32_t>(texWidth);
	m_height = static_cast<uint32_t>(texHeight);
	m_format = VK_FORMAT_R8G8B8A8_UNORM;
	VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * formatPixelSize(m_format));  // TODO: compute correctly

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	if (!m_device->createBufferAndMemory(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory))
		return false;

	void* data;
	vkMapMemory(m_device->handle(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_device->handle(), stagingBufferMemory);

	stbi_image_free(pixels);

	if (!create2D(
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight),
		m_mipLevels,
		m_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;

	transitionLayout(queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(queue, stagingBuffer, 0);
	//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	//transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
	if (generateMips)
		generateMipmaps(queue, 0);
	else
		transitionLayoutInternal(queue, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_device->destroyBuffer(stagingBuffer);
	m_device->freeDeviceMemory(stagingBufferMemory);

	return true;
}

bool Image::create2D(const std::string& filename, Queue& queue) {
	m_type = Type::eTexture2D;

	FILE* f = NULL;
	fopen_s(&f, filename.c_str(), "rb");
	if (f == NULL)
		return false;

	// see https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
	DWORD dwMagic;
	fread(&dwMagic, sizeof(DWORD), 1, f);

	if (dwMagic != 0x20534444) {
		fclose(f);
		return false;
	}

	DDS_HEADER header;
	fread(&header, sizeof(DDS_HEADER), 1, f);

	// TODO: support more formats
	if (header.ddspf.dwFourCC == 116) {
		// 116 is DXGI_FORMAT_R32G32B32A32_FLOAT
		m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	else if (header.ddspf.dwFourCC == 113) {
		// 116 is DXGI_FORMAT_R16G16B16A16_FLOAT
		m_format = VK_FORMAT_R16G16B16A16_SFLOAT;
	}
	else {
		fclose(f);
		return false;
	}

	// TODO: weird test, verify the documentation
	if (header.ddspf.dwRBitMask != 0x00ff0000 ||
		header.ddspf.dwGBitMask != 0x0000ff00 ||
		header.ddspf.dwBBitMask != 0x000000ff ||
		header.ddspf.dwABitMask != 0x00000000) {
		// format isn't RGBA - not supported for now
		fclose(f);
		return false;
	}

	m_width = static_cast<uint32_t>(header.dwWidth);
	m_height = static_cast<uint32_t>(header.dwHeight);
	m_mipLevels = static_cast<uint32_t>(header.dwMipMapCount);
	if (!create2D(
		m_width,
		m_height,
		m_mipLevels,
		m_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;

	VkDeviceSize fullImageSize = static_cast<VkDeviceSize>(m_width * m_height * formatPixelSize(m_format));
	uint8_t* pixels = new uint8_t[fullImageSize];
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	if (!m_device->createBufferAndMemory(
		fullImageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory))
		return false;

	// transition everything to dst transfer
	{
		VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_image)
			.setBaseMipLevel(0, 0)
			.setLevelCount(0, m_mipLevels)
			.setBaseLayer(0, 0)
			.setLayerCount(0, 1)
			.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		queue.endSingleTimeCommands(commandBuffer);
	}

	uint32_t width = m_width;
	uint32_t height = m_height;
	for (uint32_t mip = 0; mip < m_mipLevels; ++mip) {
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * formatPixelSize(m_format));
		size_t read = 0;
		while (read < imageSize) {
			read += fread(pixels + read, sizeof(uint8_t), imageSize - read, f);
		}

		void* data;
		vkMapMemory(m_device->handle(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_device->handle(), stagingBufferMemory);

		VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = mip;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingBuffer,
			m_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		queue.endSingleTimeCommands(commandBuffer);
		width /= 2;
		height /= 2;
	}

	m_device->destroyBuffer(stagingBuffer);
	m_device->freeDeviceMemory(stagingBufferMemory);

	delete[] pixels;
	fclose(f);

	// transition everything to shader read
	{
		VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_image)
			.setLevelCount(0, m_mipLevels)
			.setBaseLayer(0, 0)
			.setLayerCount(0, 1)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		queue.endSingleTimeCommands(commandBuffer);
	}

	return true;
}

bool Image::createCube(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
	m_type = Type::eTextureCube;
	m_width = width;
	m_height = height;
	m_mipLevels = mipLevels;
	m_format = format;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = m_width;
	imageInfo.extent.height = m_height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = m_mipLevels;
	imageInfo.arrayLayers = 6;
	imageInfo.format = m_format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	if (vkCreateImage(m_device->handle(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
		std::cerr << "failed to create image!" << std::endl;
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device->handle(), m_image, &memRequirements);

	m_imageMemory = m_device->allocateMemory(memRequirements, properties);
	if (m_imageMemory == VK_NULL_HANDLE)
		return false;

	vkBindImageMemory(m_device->handle(), m_image, m_imageMemory, 0);

	return createView(getAspect(m_format));
}

bool Image::createCube(
	const std::string& filenamePosX,
	const std::string& filenameNegX,
	const std::string& filenamePosY,
	const std::string& filenameNegY,
	const std::string& filenamePosZ,
	const std::string& filenameNegZ,
	Queue& queue,
	bool generateMips) {

	m_type = Type::eTextureCube;

	const std::string* filenames[6] = {
		&filenamePosX,
		&filenameNegX,
		&filenamePosY,
		&filenameNegY,
		&filenamePosZ,
		&filenameNegZ
	};

	VkDeviceSize imageSize = 0;
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	for (int i = 0; i < 6; ++i) {
		int texWidth, texHeight, texChannels;

		void* pixels = nullptr;
		if (stbi_is_hdr(filenames[i]->c_str())) {
			m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
			pixels = stbi_loadf(filenames[i]->c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		}
		else {
			m_format = VK_FORMAT_R8G8B8A8_SRGB;
			pixels = stbi_load(filenames[i]->c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		}

		if (!pixels) {
			std::cerr << "failed to load texture image!" << std::endl;
			return false;
		}

		if (i == 0) {
			imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * formatPixelSize(m_format));
			m_mipLevels = generateMips ? static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;
			m_width = static_cast<uint32_t>(texWidth);
			m_height = static_cast<uint32_t>(texHeight);

			// create the staging buffer
			if (!m_device->createBufferAndMemory(
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory))
				return false;

			// once we have the size, create the texture
			if (!createCube(
				m_width,
				m_height,
				m_mipLevels,
				m_format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
				return false;

			// transition everything to dst transfer
			{
				VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();
				TransitionImageBarrierBuilder<1> transition;
				transition
					.setImage(0, m_image)
					.setBaseMipLevel(0, 0)
					.setLevelCount(0, m_mipLevels)
					.setBaseLayer(0, 0)
					.setLayerCount(0, 1)
					.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
					.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
					.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
				queue.endSingleTimeCommands(commandBuffer);
			}
		}
		else if (static_cast<uint32_t>(texWidth) != m_width || static_cast<uint32_t>(texHeight) != m_height) {
			stbi_image_free(pixels);

			m_device->destroyBuffer(stagingBuffer);
			m_device->freeDeviceMemory(stagingBufferMemory);

			return false;
		}

		void* data;
		vkMapMemory(m_device->handle(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_device->handle(), stagingBufferMemory);

		stbi_image_free(pixels);

		copyBufferToImage(queue, stagingBuffer, i);

		// TODO: it is maybe possible to generate the mips for all the faces at the same time
		if (generateMips)
			generateMipmaps(queue, i);
	}

	// transition all to shader read
	if (!generateMips) {
		VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_image)
			.setBaseMipLevel(0, 0)
			.setLevelCount(0, m_mipLevels)
			.setBaseLayer(0, 0)
			.setLayerCount(0, 6)
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);  // top of pipe?
		queue.endSingleTimeCommands(commandBuffer);
	}

	m_device->destroyBuffer(stagingBuffer);
	m_device->freeDeviceMemory(stagingBufferMemory);

	return true;
}

bool Image::createCube(const std::string& filename, Queue& queue) {
	FILE* f = NULL;
	fopen_s(&f, filename.c_str(), "rb");
	if (f == NULL)
		return false;

	// see https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
	DWORD dwMagic;
	fread(&dwMagic, sizeof(DWORD), 1, f);

	if (dwMagic != 0x20534444) {
		fclose(f);
		return false;
	}

	DDS_HEADER header;
	fread(&header, sizeof(DDS_HEADER), 1, f);

	if (header.ddspf.dwFourCC == 116) {
		// 116 is DXGI_FORMAT_R32G32B32A32_FLOAT
		m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	else {
		fclose(f);
		return false;
	}

	// TODO: weird test, verify the documentation
	if (header.ddspf.dwRBitMask != 0x00ff0000 ||
		header.ddspf.dwGBitMask != 0x0000ff00 ||
		header.ddspf.dwBBitMask != 0x000000ff ||
		header.ddspf.dwABitMask != 0x00000000) {
		// format isn't RGBA - not supported for now
		fclose(f);
		return false;
	}

	m_width = static_cast<uint32_t>(header.dwWidth);
	m_height = static_cast<uint32_t>(header.dwHeight);
	m_mipLevels = static_cast<uint32_t>(header.dwMipMapCount);
	if (!createCube(
		m_width,
		m_height,
		m_mipLevels,
		m_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;

	// transition everything to dst transfer
	{
		VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_image)
			.setBaseMipLevel(0, 0)
			.setLevelCount(0, m_mipLevels)
			.setBaseLayer(0, 0)
			.setLayerCount(0, 6)
			.setLayouts(0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		queue.endSingleTimeCommands(commandBuffer);
	}

	VkDeviceSize fullImageSize = static_cast<VkDeviceSize>(m_width * m_height * formatPixelSize(m_format));
	uint8_t* pixels = new uint8_t[fullImageSize];
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	if (!m_device->createBufferAndMemory(
		fullImageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory))
		return false;

	for (uint32_t i = 0; i < 6; ++i) {
		uint32_t width = m_width;
		uint32_t height = m_height;
		for (uint32_t mip = 0; mip < m_mipLevels; ++mip) {
			VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * formatPixelSize(m_format));
			size_t read = 0;
			while (read < imageSize) {
				read += fread(pixels + read, sizeof(uint8_t), imageSize - read, f);
			}

			void* data;
			vkMapMemory(m_device->handle(), stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vkUnmapMemory(m_device->handle(), stagingBufferMemory);

			VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = mip;
			region.imageSubresource.baseArrayLayer = i;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				width,
				height,
				1
			};

			vkCmdCopyBufferToImage(
				commandBuffer,
				stagingBuffer,
				m_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);

			queue.endSingleTimeCommands(commandBuffer);
			width /= 2;
			height /= 2;
		}

	}

	m_device->destroyBuffer(stagingBuffer);
	m_device->freeDeviceMemory(stagingBufferMemory);

	delete[] pixels;
	fclose(f);

	// transition everything to shader read
	{
		VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();
		TransitionImageBarrierBuilder<1> transition;
		transition
			.setImage(0, m_image)
			.setBaseLayer(0, 0)
			.setLayerCount(0, 6)
			.setBaseMipLevel(0, 0)
			.setLevelCount(0, m_mipLevels)
			.setAspectMask(0, VK_IMAGE_ASPECT_COLOR_BIT)
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		queue.endSingleTimeCommands(commandBuffer);
	}

	return true;
}

bool Image::createView(VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.format = m_format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = m_mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;

	switch (m_type)
	{
	case Amano::Image::Type::eUnknown:
		return false;
	case Amano::Image::Type::eTexture2D:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.layerCount = 1;
		break;
	case Amano::Image::Type::eTextureCube:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.subresourceRange.layerCount = 6;
		break;
	default:
		break;
	}

	if (vkCreateImageView(m_device->handle(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
		std::cerr << "failed to create texture image view!" << std::endl;
		return false;
	}

	return true;
}

bool Image::createSampler(VkFilter magFilter, VkFilter minFilter) {
	SamplerBuilder samplerBuilder;
	samplerBuilder
		.setFilter(magFilter, minFilter)
		.setMaxLod((float)(m_mipLevels - 1));
	m_imageSampler = samplerBuilder.build(*m_device);

	return true;
}

void Image::transitionLayout(Queue& queue, VkImageLayout oldLayout, VkImageLayout newLayout) {
	transitionLayoutInternal(queue, 0, oldLayout, newLayout);
}

void Image::transitionLayoutInternal(Queue& queue, uint32_t layer, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();

	TransitionImageBarrierBuilder<1> transition;
	transition
		.setImage(0, m_image)
		.setLevelCount(0, m_mipLevels)
		.setBaseLayer(0, layer)
		.setLayerCount(0, 1)
		.setLayouts(0, oldLayout, newLayout)
		.setAspectMask(0, getAspect(m_format));

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		transition.setAccessMasks(0, 0, VK_ACCESS_TRANSFER_WRITE_BIT);

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		transition.setAccessMasks(0, 0, VK_ACCESS_TRANSFER_READ_BIT);

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		transition.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		transition.setAccessMasks(0, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		transition.setAccessMasks(0, 0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		transition.setAccessMasks(0, 0, VK_ACCESS_SHADER_READ_BIT);

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else {
		std::cerr << "unsupported layout transition!" << std::endl;
		return;
	}

	transition.execute(commandBuffer, sourceStage, destinationStage);

	queue.endSingleTimeCommands(commandBuffer);
}

void Image::copyBufferToImage(Queue& queue, VkBuffer buffer, uint32_t layer) {
	VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = layer;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		m_width,
		m_height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		m_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	queue.endSingleTimeCommands(commandBuffer);
}

void Image::generateMipmaps(Queue& queue, uint32_t layer) {
	if (!m_device->doesSuportBlitting(m_format))
		return;

	VkCommandBuffer commandBuffer = queue.beginSingleTimeCommands();

	TransitionImageBarrierBuilder<1> transition;
	transition
		.setImage(0, m_image)
		.setBaseLayer(0, layer);

	int32_t mipWidth = static_cast<int32_t>(m_width);
	int32_t mipHeight = static_cast<int32_t>(m_height);

	// All mip levels are VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	
	for (uint32_t i = 1; i < m_mipLevels; i++) {
		// set (i-1) mip level:
		//   transfer dst -> transfer src
		//   transfer write -> transfer read
		transition
			.setBaseMipLevel(0, i - 1)
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = layer;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = layer;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		// set (i-1) mip level:
		//   transfer src -> shader read optimal
		//   transfer read -> shader read
		transition
			.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.setAccessMasks(0, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT)
			.execute(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	// set lowest mip level:
	//   transfer dst -> shader read optimal
	//   transfer write -> shader read
	transition
		.setBaseMipLevel(0, m_mipLevels - 1)
		.setLayouts(0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		.setAccessMasks(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
		.execute(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	queue.endSingleTimeCommands(commandBuffer);
}

}