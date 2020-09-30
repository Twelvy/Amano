#include "PipelineBuilder.h"

#include <array>
#include <fstream>
#include <iostream>

namespace Amano {

PipelineBuilder::PipelineBuilder(Device* device)
	: PipelineBuilderBase(device)
	, m_viewport {}
	, m_scissor{}
	, m_rasterizer{}
{
	// set default
	m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizer.depthClampEnable = VK_FALSE;
	m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizer.lineWidth = 1.0f;
	m_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	m_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizer.depthBiasEnable = VK_FALSE;
	m_rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	m_rasterizer.depthBiasClamp = 0.0f; // Optional
	m_rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
}

PipelineBuilder::~PipelineBuilder() {
}

PipelineBuilder& PipelineBuilder::addShader(const std::string& filename, VkShaderStageFlagBits stage) {
	addShaderBase(filename, stage);
	return *this;
}

PipelineBuilder& PipelineBuilder::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.width = width;
	m_viewport.height = height;
	m_viewport.minDepth = minDepth;
	m_viewport.maxDepth = maxDepth;

	return *this;
}

PipelineBuilder& PipelineBuilder::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
	m_scissor.offset.x = x;
	m_scissor.offset.y = y;
	m_scissor.extent.width = width;
	m_scissor.extent.height = height;

	return *this;
}

PipelineBuilder& PipelineBuilder::setRasterizer(VkCullModeFlagBits cullMode, VkFrontFace frontface) {
	m_rasterizer.cullMode = cullMode;
	m_rasterizer.frontFace = frontface;

	return *this;
}

VkPipeline PipelineBuilder::build(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, uint32_t subpass, uint32_t renderTargetCount, bool hasDepth) {

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // we only support triangles for now
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &m_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &m_scissor;

	// we don't support multisampling for now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(renderTargetCount);
	for (uint32_t i = 0; i < renderTargetCount; ++i) {
		blendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachments[i].blendEnable = VK_FALSE;
		blendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		blendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		blendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD; // Optional
		blendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		blendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		blendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	}

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
	colorBlending.pAttachments = blendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	if (hasDepth) {
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		//depthStencil.front{}; // Optional
		//depthStencil.back{}; // Optional
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	pipelineInfo.pStages = m_shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &m_rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	if (hasDepth)
		pipelineInfo.pDepthStencilState = &depthStencil;
	else
		pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	VkPipeline pipeline = VK_NULL_HANDLE;
	if (vkCreateGraphicsPipelines(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		std::cerr << "failed to create graphics pipeline!" << std::endl;
	}

	return pipeline;
}

}
