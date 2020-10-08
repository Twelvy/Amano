#pragma once

#include "glfw.h"
#include "Device.h"

#include <imgui.h>

namespace Amano {

class ImguiSystem
{
public:
	ImguiSystem(Device* device);
	~ImguiSystem();

	VkRenderPass renderPass() { return m_renderPass; }

	bool init();

	void updateMouse(GLFWwindow* window);
	bool hasCapturedMouse();
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
	bool m_mouseJustPressed[ImGuiMouseButton_COUNT];
};

}
