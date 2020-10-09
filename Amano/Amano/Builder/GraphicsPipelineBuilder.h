#pragma once

#include "../Device.h"
#include "PipelineBuilderBase.h"

#include <string>
#include <vector>

namespace Amano {

class GraphicsPipelineBuilder : public PipelineBuilderBase
{
public:
	GraphicsPipelineBuilder(Device* device);
	~GraphicsPipelineBuilder();

	GraphicsPipelineBuilder& addShader(const std::string& filename, VkShaderStageFlagBits stage);

	GraphicsPipelineBuilder& setViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
	GraphicsPipelineBuilder& setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

	GraphicsPipelineBuilder& setRasterizer(VkCullModeFlagBits cullMode, VkFrontFace frontface);

	VkPipeline build(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, uint32_t subpass, uint32_t renderTargetCount, bool hasDepth);

private:
	VkViewport m_viewport;
	VkRect2D m_scissor;
	VkPipelineRasterizationStateCreateInfo m_rasterizer;
};

}
