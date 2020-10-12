#pragma once

#include "glfw.h"
#include "glm.h"
#include "DebugOrbitCamera.h"
#include "Device.h"
#include "Image.h"
#include "InputSystem.h"
#include "Mesh.h"
#include "Ubo.h"
#include "UniformBuffer.h"
#include "Builder/RaytracingAccelerationStructureBuilder.h"
#include "Builder/ShaderBindingTableBuilder.h"
#include "Pass/BlitToSwapChainPass.h"
#include "Pass/DeferredLightingPass.h"
#include "Pass/GBufferPass.h"
#include "Pass/ImGuiSystem.h"
#include "Pass/RaytracingShadowPass.h"
#include "Pass/ToneMappingPass.h"

namespace Amano {

class Application {
public:
	Application();
	~Application();

	// Initializes the window, Vulkan and the sampe application
	bool init();

	// Starts the application
	// the function will return when the window closes
	void run();

	// Receives the new size of the window
	// There is no need to call it manually
	void notifyFramebufferResized(int width, int height);

	// Receives the scroll input
	// There is no need to call it manually
	void onScrollCallback(double xscroll, double yscroll);

private:
	void initWindow();

	void recreateSwapChain();
	void createSizeDependentObjects();
	void cleanSizedependentObjects();

	void drawFrame();
	void drawUI(uint32_t imageIndex);
	void updateUniformBuffers();

private:
	GLFWwindow* m_window;

	// Flag indicating the window has been resized. Not used for now
	bool m_framebufferResized;

	// The width of the window
	uint32_t m_width;

	// The height of the window
	uint32_t m_height;

	Device* m_device;

	InputSystem* m_inputSystem;
	ImGuiSystem* m_guiSystem;
	DebugOrbitCamera* m_debugOrbitCamera;

	// the information for the sample is here
	// All of this should be wrapped into proper classes for easy access
	Mesh* m_mesh;
	Image* m_modelTexture;
	VkSemaphore m_imageAvailableSemaphore;
	VkFence m_inFlightFence;
	// need more for double buffering

	GBufferPass* m_gBufferPass;

	// for lighting shader
	DeferredLightingPass* m_deferredLightingPass;

	// for raytracing
	RaytracingShadowPass* m_raytracingPass;

	// for tone mapping
	ToneMappingPass* m_toneMappingPass;

	// for blit
	BlitToSwapChainPass* m_blitToSwapChainPass;

	// light information
	glm::vec3 m_lightPosition;
};

}
