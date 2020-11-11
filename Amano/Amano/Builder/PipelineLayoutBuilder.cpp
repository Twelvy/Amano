#include "PipelineLayoutBuilder.h"

#include <iostream>

#include <array>
#include <fstream>
#include <iostream>

namespace Amano {

PipelineLayoutBuilder::PipelineLayoutBuilder()
	: m_descriptorSetLayouts()
	, m_pushConstantRanges()
{
}

PipelineLayoutBuilder& PipelineLayoutBuilder::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
	m_descriptorSetLayouts.push_back(layout);

	return *this;
}

PipelineLayoutBuilder& PipelineLayoutBuilder::addPushConstantRange(VkPushConstantRange range) {
	m_pushConstantRanges.push_back(range);

	return *this;
}

VkPipelineLayout PipelineLayoutBuilder::build(Device& device) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = m_pushConstantRanges.data();

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(device.handle(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		std::cerr << "failed to create pipeline layout!" << std::endl;
	}

	return pipelineLayout;
}

}