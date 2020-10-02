#include "DescriptorSetBuilder.h"

#include <iostream>

namespace Amano {

Descriptor::Descriptor(VkBuffer buffer, VkDeviceSize range, uint32_t binding)
	: m_type{ DescriptorType::eBuffer }
	, m_binding{ binding }
{
	m_bufferInfo.buffer = buffer;
	m_bufferInfo.offset = 0;
	m_bufferInfo.range = range; // or VK_WHOLE_SIZE
}

Descriptor::Descriptor(VkSampler sampler, VkImageView imageView, uint32_t binding)
	: m_type{ DescriptorType::eImage }
	, m_binding{ binding }
{
	m_imageInfo.sampler = sampler;
	m_imageInfo.imageView = imageView;
	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

Descriptor::Descriptor(VkImageView imageView, uint32_t binding)
	: m_type{ DescriptorType::eStorageImage }
	, m_binding{ binding }
{
	m_imageInfo.sampler = VK_NULL_HANDLE;
	m_imageInfo.imageView = imageView;
	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
}

Descriptor::Descriptor(VkAccelerationStructureKHR* acc, uint32_t binding)
	: m_type{ DescriptorType::eAccelerationStructure }
	, m_binding{ binding }
{
	m_accelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	m_accelerationStructure.pNext = nullptr;
	m_accelerationStructure.accelerationStructureCount = 1;
	m_accelerationStructure.pAccelerationStructures = acc;
}

void Descriptor::set(VkWriteDescriptorSet& writeDescriptor, VkDescriptorSet descriptorSet) {
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.pNext = nullptr;
	writeDescriptor.dstSet = descriptorSet;
	writeDescriptor.dstBinding = m_binding;
	writeDescriptor.dstArrayElement = 0; // only used with VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT 
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.pBufferInfo = nullptr; // Optional
	writeDescriptor.pImageInfo = nullptr; // Optional
	writeDescriptor.pTexelBufferView = nullptr; // Optional

	switch (m_type)
	{
	case Amano::Descriptor::eBuffer:
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptor.pBufferInfo = &m_bufferInfo;
		break;
	case Amano::Descriptor::eImage:
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptor.pImageInfo = &m_imageInfo;
		break;
	case Amano::Descriptor::eStorageImage:
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writeDescriptor.pImageInfo = &m_imageInfo;
		break;
	case Amano::Descriptor::eAccelerationStructure:
		writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		writeDescriptor.pNext = &m_accelerationStructure;
		break;
	default:
		break;
	}
}


DescriptorSetBuilder::DescriptorSetBuilder(Device* device, uint32_t count, VkDescriptorSetLayout layout)
	: m_device{ device }
	, m_descriptors()
{
	std::vector<VkDescriptorSetLayout> layouts(2, layout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = device->getDescriptorPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(m_device->handle(), &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
		std::cerr << "failed to allocate descriptor sets!" << std::endl;
	}
}

DescriptorSetBuilder& DescriptorSetBuilder::addUniformBuffer(VkBuffer buffer, VkDeviceSize range, uint32_t binding) {
	m_descriptors.emplace_back(buffer, range, binding);

	return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::addImage(VkSampler sampler, VkImageView imageView, uint32_t binding) {
	m_descriptors.emplace_back(sampler, imageView, binding);

	return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::addStorageImage(VkImageView imageView, uint32_t binding) {
	m_descriptors.emplace_back(imageView, binding);

	return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::addAccelerationStructure(VkAccelerationStructureKHR* acc, uint32_t binding) {
	m_descriptors.emplace_back(acc, binding);

	return *this;
}

VkDescriptorSet DescriptorSetBuilder::buildAndUpdate() {
	std::vector<VkWriteDescriptorSet> descriptorWrites(m_descriptors.size());
	for (int i = 0; i < m_descriptors.size(); ++i) {
		m_descriptors[i].set(descriptorWrites[i], m_descriptorSet);
	}
	vkUpdateDescriptorSets(m_device->handle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	return m_descriptorSet;
}

}