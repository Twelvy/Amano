#pragma once

#include "../Device.h"
#include "PipelineBuilderBase.h"

#include <string>
#include <vector>

namespace Amano {

class RaytracingPipelineBuilder : public PipelineBuilderBase
{
public:
	RaytracingPipelineBuilder(Device* device);
	~RaytracingPipelineBuilder();

	RaytracingPipelineBuilder& addShader(const std::string& filename, VkShaderStageFlagBits stage);

	VkPipeline build(VkPipelineLayout pipelineLayout, uint32_t maxRecursion);

private:
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
};

}
