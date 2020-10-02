#include "SamplerBuilder.h"

#include <iostream>

namespace Amano {

SamplerBuilder::SamplerBuilder()
	: m_samplerInfo{}
{
	m_samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	m_samplerInfo.magFilter = VK_FILTER_LINEAR;
	m_samplerInfo.minFilter = VK_FILTER_LINEAR;
	m_samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	m_samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	m_samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	m_samplerInfo.anisotropyEnable = VK_TRUE;
	m_samplerInfo.maxAnisotropy = 16.0f;
	m_samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	m_samplerInfo.unnormalizedCoordinates = VK_FALSE;
	m_samplerInfo.compareEnable = VK_FALSE;
	m_samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	m_samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	m_samplerInfo.mipLodBias = 0.0f;
	m_samplerInfo.minLod = 0.0f;
	m_samplerInfo.maxLod = 0.0f;
}

SamplerBuilder& SamplerBuilder::setMaxLoad(float maxLod) {
	m_samplerInfo.maxLod = maxLod;

	return *this;
}


VkSampler SamplerBuilder::build(Device& device) {
	VkSampler sampler = VK_NULL_HANDLE;
	if (vkCreateSampler(device.handle(), &m_samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		std::cerr << "failed to create texture sampler!" << std::endl;
	}

	return sampler;
}

}
