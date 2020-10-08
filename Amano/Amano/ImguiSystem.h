#pragma once

#include "Device.h"

namespace Amano {

class ImguiSystem
{
public:
	ImguiSystem(Device* device);
	~ImguiSystem();

	VkRenderPass renderPass() { return m_renderPass; }

	bool init();

	void startFrame();
	void endFrame(VkFramebuffer framebuffer, uint32_t width, uint32_t height);

private:
	bool createRenderPass();
	bool createDescriptorPool();
	bool initImgui();

private:
	Device* m_device;
	VkDescriptorPool m_descriptorPool;
	VkRenderPass m_renderPass;
};

}
