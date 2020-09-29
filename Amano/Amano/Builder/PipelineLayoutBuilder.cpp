#include "PipelineLayoutBuilder.h"

#include <iostream>

#include <array>
#include <fstream>
#include <iostream>

namespace {
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			std::cerr << "failed to open file!" << std::endl;
			std::vector<char> buffer;
			return buffer;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
}

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