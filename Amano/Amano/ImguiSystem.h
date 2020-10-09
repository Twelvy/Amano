#pragma once

#include "glfw.h"
#include "Device.h"
#include "InputSystem.h"

#include <imgui.h>

namespace Amano {

class ImguiSystem : public InputReader
{
public:
	ImguiSystem(Device* device);
	~ImguiSystem();

	VkRenderPass renderPass() { return m_renderPass; }

	bool init();

	// methods for InputReader
	bool updateMouse(GLFWwindow* window) final;
	bool updateScroll(double, double) final { return false; }

	// Call this method to start recording the UI 
	void startFrame();

	// Ends the recording of the UI and submits the rendering command
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
