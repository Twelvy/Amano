#include "Config.h"
#include "FramebufferBuilder.h"

#include <iostream>

namespace Amano {

FramebufferBuilder::FramebufferBuilder()
	: m_attachments()
{
}

FramebufferBuilder& FramebufferBuilder::addAttachment(VkImageView imageView) {
	m_attachments.push_back(imageView);

	return *this;
}

VkFramebuffer FramebufferBuilder::build(Device& device, VkRenderPass renderPass, uint32_t width, uint32_t height) {
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
	framebufferInfo.pAttachments = m_attachments.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	if (vkCreateFramebuffer(device.handle(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
		std::cerr << "failed to create framebuffer!" << std::endl;
	}

	return framebuffer;
}

}