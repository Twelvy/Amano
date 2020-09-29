#pragma once

#include "../Config.h"
#include "../Device.h"

#include <vector>

namespace Amano {

class DescriptorSetLayoutBuilder
{
public:
	DescriptorSetLayoutBuilder();

	DescriptorSetLayoutBuilder& addBinding(VkDescriptorType type, VkShaderStageFlags stageFlags);

	VkDescriptorSetLayout build(Device& device);

private:
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

}
