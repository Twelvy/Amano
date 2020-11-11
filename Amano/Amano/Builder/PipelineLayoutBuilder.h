#pragma once

#include "../Device.h"

#include <vector>

namespace Amano {

class PipelineLayoutBuilder
{
public:
	PipelineLayoutBuilder();
	
	PipelineLayoutBuilder& addDescriptorSetLayout(VkDescriptorSetLayout layout);
	PipelineLayoutBuilder& addPushConstantRange(VkPushConstantRange range);

	VkPipelineLayout build(Device& device);

private:
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkPushConstantRange> m_pushConstantRanges;
};

}
