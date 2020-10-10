#pragma once

#include "../Device.h"
#include "PipelineBuilderBase.h"

namespace Amano {

class ComputePipelineBuilder : public PipelineBuilderBase
{
public:
	ComputePipelineBuilder(Device* device);
	~ComputePipelineBuilder();

	ComputePipelineBuilder& addShader(const std::string& filename, VkShaderStageFlagBits stage);

	VkPipeline build(VkPipelineLayout pipelineLayout);

private:
};

}
