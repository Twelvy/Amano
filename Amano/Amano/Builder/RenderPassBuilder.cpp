#include "RenderPassBuilder.h"

#include <iostream>

namespace Amano {

RenderPassBuilder::RenderPassBuilder()
	: m_attachments()
	, m_subpasses()
	, m_dependencies()
{
}

RenderPassBuilder& RenderPassBuilder::addColorAttachment(VkFormat format, VkImageLayout finalLayout) {
	VkAttachmentDescription& colorAttachment = m_attachments.emplace_back();
	colorAttachment.format = format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = finalLayout;

	return *this;
}

RenderPassBuilder& RenderPassBuilder::addDepthAttachment(VkFormat format, VkImageLayout finalLayout) {
	VkAttachmentDescription& depthAttachment = m_attachments.emplace_back();
	depthAttachment.format = format;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = finalLayout;

	return *this;
}

RenderPassBuilder& RenderPassBuilder::addSubpass(VkPipelineBindPoint bindPoint, std::vector<uint32_t> colorAttachmentIndices, uint32_t depthAttachmentIndex) {
	auto& subpass = m_subpasses.emplace_back(colorAttachmentIndices.size());

	for (size_t i = 0; i < colorAttachmentIndices.size(); ++i) {
		subpass.colorAttachmentReferences[i].attachment = colorAttachmentIndices[i];
		subpass.colorAttachmentReferences[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	subpass.desc.pipelineBindPoint = bindPoint;
	subpass.desc.colorAttachmentCount = static_cast<uint32_t>(subpass.colorAttachmentReferences.size());
	subpass.desc.pColorAttachments = subpass.colorAttachmentReferences.data();
	if (depthAttachmentIndex >= 0) {
		subpass.depthAttachmentReference.attachment = depthAttachmentIndex;
		subpass.depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		subpass.desc.pDepthStencilAttachment = &subpass.depthAttachmentReference;
	}
	else {
		subpass.desc.pDepthStencilAttachment = nullptr;
	}

	return *this;
}

RenderPassBuilder& RenderPassBuilder::addSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass) {
	auto& dependency = m_dependencies.emplace_back();
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = dstSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	return *this;
}

VkRenderPass RenderPassBuilder::build(Device& device) {
	std::vector<VkSubpassDescription> subpasses;
	subpasses.reserve(m_subpasses.size());
	for (auto& desc : m_subpasses) {
		subpasses.push_back(desc.desc);
	}

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
	renderPassInfo.pAttachments = m_attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(m_dependencies.size());
	renderPassInfo.pDependencies = m_dependencies.data();

	VkRenderPass renderPass = VK_NULL_HANDLE;
	if (vkCreateRenderPass(device.handle(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		std::cerr << "failed to create render pass!" << std::endl;
	}

	return renderPass;
}

}
