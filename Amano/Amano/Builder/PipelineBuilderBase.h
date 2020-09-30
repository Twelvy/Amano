#pragma once

#include "../Config.h"
#include "../Device.h"

#include <string>
#include <vector>

namespace Amano {

class PipelineBuilderBase
{
public:
	PipelineBuilderBase(Device& device);
	virtual ~PipelineBuilderBase();

protected:
	void addShaderBase(const std::string& filename, VkShaderStageFlagBits stage);

private:
	VkShaderModule createShaderModule(const std::vector<char>& code);

protected:
	VkDevice m_device;
	std::vector<VkShaderModule> m_shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
};

}
