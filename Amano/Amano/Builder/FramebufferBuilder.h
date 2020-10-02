#pragma once

#include "../Config.h"
#include "../Device.h"

#include <vector>

namespace Amano {

class FramebufferBuilder {
public:
	FramebufferBuilder();

	FramebufferBuilder& addAttachment(VkImageView imageView);

	VkFramebuffer build(Device& device, VkRenderPass renderPass, uint32_t width, uint32_t depth);

private:
	std::vector<VkImageView> m_attachments;
};

}
