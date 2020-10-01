#pragma once

#include "../Device.h"
#include "PipelineBuilderBase.h"

#include <string>
#include <vector>

namespace Amano {

class PipelineBuilder : public PipelineBuilderBase
{
public:
	PipelineBuilder(Device* device);
	~PipelineBuilder();

	PipelineBuilder& addShader(const std::string& filename, VkShaderStageFlagBits stage);

	PipelineBuilder& setViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
	PipelineBuilder& setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

	PipelineBuilder& setRasterizer(VkCullModeFlagBits cullMode, VkFrontFace frontface);

	VkPipeline build(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, uint32_t subpass, uint32_t renderTargetCount, bool hasDepth);

private:
	VkViewport m_viewport;
	VkRect2D m_scissor;
	VkPipelineRasterizationStateCreateInfo m_rasterizer;
};

}
