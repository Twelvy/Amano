#pragma once

#include "Pass.h"
#include "../glfw.h"
#include "../Device.h"
#include "../InputSystem.h"

#include <imgui.h>

namespace Amano {

class ImGuiSystem : public Pass, public InputReader
{
public:
	ImGuiSystem(Device* device);
	~ImGuiSystem();

	VkRenderPass renderPass() { return m_renderPass; }

	bool init();

	// methods for InputReader
	bool updateMouse(GLFWwindow* window) final;
	bool updateScroll(double, double) final { return false; }

	// Call this method to start recording the UI 
	void startFrame();

	// Ends the recording of the UI and submits the rendering command
	void endFrame(uint32_t imageIndex, uint32_t width, uint32_t height, VkFence fence);

	void cleanOnRenderTargetResized();
	void recreateOnRenderTargetResized(uint32_t width, uint32_t height);

private:
	bool createRenderPass();
	bool createDescriptorPool();
	bool initImgui();

private:
	VkDescriptorPool m_descriptorPool;
	VkRenderPass m_renderPass;
	VkCommandBuffer m_commandBuffer;
	std::vector<VkFramebuffer> m_framebuffers;
	bool m_mouseJustPressed[ImGuiMouseButton_COUNT];
};

}
