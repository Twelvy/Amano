#include "Config.h"
#include "RaytracingPipelineBuilder.h"

#include <array>
#include <fstream>
#include <iostream>

namespace Amano {

RaytracingPipelineBuilder::RaytracingPipelineBuilder(Device* device)
	: PipelineBuilderBase(device)
	, m_shaderGroups()
{
}

RaytracingPipelineBuilder::~RaytracingPipelineBuilder() {
}

RaytracingPipelineBuilder& RaytracingPipelineBuilder::addShader(const std::string& filename, VkShaderStageFlagBits stage) {
	addShaderBase(filename, stage);

	auto& shaderGroupInfo = m_shaderGroups.emplace_back();
	shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroupInfo.pNext = nullptr;
	shaderGroupInfo.generalShader = VK_SHADER_UNUSED_NV;
	shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	if (stage == VK_SHADER_STAGE_RAYGEN_BIT_NV) {
		shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		shaderGroupInfo.generalShader = static_cast<uint32_t>(m_shaderGroups.size() - 1);
	}
	else if (stage == VK_SHADER_STAGE_MISS_BIT_NV) {
		shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		shaderGroupInfo.generalShader = static_cast<uint32_t>(m_shaderGroups.size() - 1);
	}
	else if (stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV) {
		shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
		shaderGroupInfo.closestHitShader = static_cast<uint32_t>(m_shaderGroups.size() - 1);
	}

	// TODO: handle more cases

	return *this;
}

VkPipeline RaytracingPipelineBuilder::build(VkPipelineLayout pipelineLayout, uint32_t maxRecursion) {

	VkRayTracingPipelineCreateInfoNV pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	pipelineInfo.pStages = m_shaderStages.data();
	pipelineInfo.groupCount = static_cast<uint32_t>(m_shaderGroups.size());
	pipelineInfo.pGroups = m_shaderGroups.data();
	pipelineInfo.maxRecursionDepth = maxRecursion;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = 0; // Optional

	VkPipeline pipeline = VK_NULL_HANDLE;
	if (m_device->getExtensions().vkCreateRayTracingPipelinesNV(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		std::cerr << "failed to create raytracing pipeline!" << std::endl;
	}

	return pipeline;
}

}
