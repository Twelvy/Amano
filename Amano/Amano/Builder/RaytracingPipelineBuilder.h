#pragma once

#include "../Config.h"
#include "../Device.h"
#include "PipelineBuilderBase.h"

#include <string>
#include <vector>

namespace Amano {

class RaytracingPipelineBuilder : public PipelineBuilderBase
{
public:
	RaytracingPipelineBuilder(Device& device);
	~RaytracingPipelineBuilder();

	RaytracingPipelineBuilder& addShader(const std::string& filename, VkShaderStageFlagBits stage);

	//VkPipeline build(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, uint32_t subpass, uint32_t renderTargetCount, bool hasDepth);

private:
};

}
