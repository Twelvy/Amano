#include "PipelineLayoutBuilder.h"

#include <iostream>

#include <array>
#include <fstream>
#include <iostream>

namespace Amano {

PipelineLayoutBuilder::PipelineLayoutBuilder()
	: m_descriptorSetLayouts()
{
}

PipelineLayoutBuilder& PipelineLayoutBuilder::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
	m_descriptorSetLayouts.push_back(layout);

	return *this;
}

VkPipelineLayout PipelineLayoutBuilder::build(Device& device) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(device.handle(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		std::cerr << "failed to create pipeline layout!" << std::endl;
	}

	return pipelineLayout;
}

}