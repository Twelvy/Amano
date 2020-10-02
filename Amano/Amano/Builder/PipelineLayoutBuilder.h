#pragma once

#include "../Device.h"

#include <vector>

namespace Amano {

class PipelineLayoutBuilder
{
public:
	PipelineLayoutBuilder();
	
	PipelineLayoutBuilder& addDescriptorSetLayout(VkDescriptorSetLayout layout);

	VkPipelineLayout build(Device& device);

private:
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};

}
