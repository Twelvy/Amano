#pragma once

#include "../Device.h"

#include <vector>

namespace Amano {

class Descriptor {
public:
	Descriptor(VkBuffer buffer, VkDeviceSize range, uint32_t binding);
	Descriptor(VkSampler sampler, VkImageView imageView, uint32_t binding);
	Descriptor(VkImageView imageView, uint32_t binding);
	Descriptor(VkAccelerationStructureKHR* acc, uint32_t binding);

	void set(VkWriteDescriptorSet& writeDescriptor, VkDescriptorSet descriptorSet);

private:
	// More types will be added later
	enum DescriptorType {
		eBuffer,
		eImage,
		eStorageImage,
		eAccelerationStructure
	};

private:
	DescriptorType m_type;
	uint32_t m_binding;
	union {
		VkDescriptorBufferInfo m_bufferInfo;
		VkDescriptorImageInfo m_imageInfo;
		VkWriteDescriptorSetAccelerationStructureKHR m_accelerationStructure;
	};
};

class DescriptorSetBuilder {
public:
	DescriptorSetBuilder(Device* device, uint32_t count, VkDescriptorSetLayout layout);

	DescriptorSetBuilder& addUniformBuffer(VkBuffer buffer, VkDeviceSize range, uint32_t binding);
	DescriptorSetBuilder& addImage(VkSampler sampler, VkImageView imageView, uint32_t binding);
	DescriptorSetBuilder& addStorageImage(VkImageView imageView, uint32_t binding);
	DescriptorSetBuilder& addAccelerationStructure(VkAccelerationStructureKHR* acc, uint32_t binding);

	VkDescriptorSet buildAndUpdate();

private:
	

private:
	Device* m_device;
	VkDescriptorSet m_descriptorSet;
	std::vector<Descriptor> m_descriptors;
};

}
