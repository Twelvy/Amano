#pragma once

#include "../Device.h"

#include <vector>

namespace Amano {

class RenderPassBuilder
{
private:
	struct SubpassDescription
	{
		SubpassDescription(size_t colorCount)
			: desc{}
			, colorAttachmentReferences(colorCount)
			, depthAttachmentReference{}
		{
		}

		VkSubpassDescription desc;
		std::vector<VkAttachmentReference> colorAttachmentReferences;
		VkAttachmentReference depthAttachmentReference;
	};

public:
	RenderPassBuilder();

	// TODO: make those methods more versatile

	RenderPassBuilder& addColorAttachment(VkFormat format, VkImageLayout finalLayout);
	RenderPassBuilder& addDepthAttachment(VkFormat format, VkImageLayout finalLayout);

	RenderPassBuilder& addSubpass(VkPipelineBindPoint bindPoint, std::vector<uint32_t> colorAttachmentIndices, uint32_t depthAttachmentIndex);

	RenderPassBuilder& addSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass);

	VkRenderPass build(Device& device);

private:
	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<SubpassDescription> m_subpasses;
	std::vector<VkSubpassDependency> m_dependencies;
};

}
