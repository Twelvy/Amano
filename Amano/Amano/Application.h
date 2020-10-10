#pragma once

#include "glfw.h"
#include "glm.h"
#include "DebugOrbitCamera.h"
#include "Device.h"
#include "Image.h"
#include "ImGuiSystem.h"
#include "InputSystem.h"
#include "Mesh.h"
#include "UniformBuffer.h"
#include "Builder/RaytracingAccelerationStructureBuilder.h"
#include "Builder/ShaderBindingTableBuilder.h"

namespace Amano {

// Uniform buffer for gbuffer vertex shader 
struct PerFrameUniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// Uniform buffer for raygen shader
struct RayParams {
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
	glm::vec3 rayOrigin;
};

struct LightInformation {
	glm::vec3 lightPosition;
};

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

	struct Formats {
		VkFormat depthFormat;
		VkFormat colorFormat;
		VkFormat normalFormat;
		VkFormat depthFormat2;
	};
	Formats getFormats();

	void recreateSwapChain();
	void createSizeDependentObjects();
	void cleanSizedependentObjects();

	void drawFrame();
	void drawUI(uint32_t imageIndex);
	void updateUniformBuffer();

	// multiple methods to record the commands once
	void recordRenderCommands();
	void recordComputeCommands();
	void recordRaytracingCommands();
	void recordBlitCommands();

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
	std::vector<VkFramebuffer> m_finalFramebuffers;

	// the information for the sample is here
	// All of this should be wrapped into proper classes for easy access
	Image* m_depthImage;
	VkSampler m_nearestSampler;
	struct {
		Image* colorImage = nullptr;
		Image* normalImage = nullptr;
	} m_GBuffer;
	VkRenderPass m_renderPass;
	VkFramebuffer m_framebuffer;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	UniformBuffer<PerFrameUniformBufferObject>* m_uniformBuffer;
	UniformBuffer<LightInformation>* m_lightUniformBuffer;
	Mesh* m_mesh;
	Image* m_modelTexture;
	VkSampler m_sampler;
	VkDescriptorSet m_descriptorSet;
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkSemaphore m_lightingFinishedSemaphore;
	VkSemaphore m_raytracingFinishedSemaphore;
	VkSemaphore m_blitFinishedSemaphore;
	VkFence m_inFlightFence;
	VkFence m_lightingFence;
	VkFence m_raytracingFence;
	VkFence m_blitFence;
	// need more for double buffering
	VkCommandBuffer m_renderCommandBuffer;
	std::vector<VkCommandBuffer> m_blitCommandBuffers;

	// for compute shader
	VkDescriptorSetLayout m_computeDescriptorSetLayout;
	VkPipelineLayout m_computePipelineLayout;
	VkPipeline m_computePipeline;
	VkDescriptorSet m_computeDescriptorSet;
	Image* m_computeImage;
	VkCommandBuffer m_computeCommandBuffer;

	// for raytracing
	Image* m_raytracingImage;
	UniformBuffer<RayParams>* m_raytracingUniformBuffer;
	VkDescriptorSetLayout m_raytracingDescriptorSetLayout;
	VkPipelineLayout m_raytracingPipelineLayout;
	VkPipeline m_raytracingPipeline;
	VkDescriptorSet m_raytracingDescriptorSet;
	AccelerationStructures m_accelerationStructures;
	ShaderBindingTables m_shaderBindingTables;
	VkCommandBuffer m_raytracingCommandBuffer;

	// light information
	glm::vec3 m_lightPosition;
};

}
