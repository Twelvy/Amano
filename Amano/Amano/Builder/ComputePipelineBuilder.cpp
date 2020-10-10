#include "ComputePipelineBuilder.h"
#include "../Vertex.h"

#include <array>
#include <fstream>
#include <iostream>

namespace Amano {

ComputePipelineBuilder::ComputePipelineBuilder(Device* device)
	: PipelineBuilderBase(device)
{
}

ComputePipelineBuilder::~ComputePipelineBuilder() {
}

ComputePipelineBuilder& ComputePipelineBuilder::addShader(const std::string& filename, VkShaderStageFlagBits stage) {
	addShaderBase(filename, stage);
	return *this;
}

VkPipeline ComputePipelineBuilder::build(VkPipelineLayout pipelineLayout) {

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stage = m_shaderStages[0];
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
	pipelineInfo.basePipelineIndex = 0;  // Optional
	
	VkPipeline pipeline = VK_NULL_HANDLE;
	if (vkCreateComputePipelines(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		std::cerr << "failed to create graphics pipeline!" << std::endl;
	}

	return pipeline;
}

}
