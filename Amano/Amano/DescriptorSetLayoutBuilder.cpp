#include "DescriptorSetLayoutBuilder.h"

#include <iostream>

namespace Amano {

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder()
	: m_bindings()
{
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addBinding(VkDescriptorType type, VkShaderStageFlags stageFlags) {
	auto& binding = m_bindings.emplace_back();
	binding.binding = static_cast<uint32_t>(m_bindings.size() - 1);
	binding.descriptorType = type;
	binding.descriptorCount = 1;
	binding.stageFlags = stageFlags;
	binding.pImmutableSamplers = nullptr; // Optional

	return *this;
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::build(Device& device) {
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
	layoutInfo.pBindings = m_bindings.data();

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	if (vkCreateDescriptorSetLayout(device.handle(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		std::cerr << "failed to create descriptor set layout!" << std::endl;
	}

	return descriptorSetLayout;
}

}
