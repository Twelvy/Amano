#pragma once

#include "Device.h"
#include "Image.h"
#include "Model.h"
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

	// Receives the keyboard input
	// There is no need to call it manually
	void onKeyEventCallback(int key, int scancode, int action, int mods);

private:
	void initWindow();

	void drawFrame();
	void updateUniformBuffer();

	// multiple methods to record the commands once
	void recordRenderCommands();
	void recordBlitCommands();
	void recordRaytracingCommands();

	// Temporary method to create the data and graphics object needed for raytracing
	void setupRaytracingData();

private:
	GLFWwindow* m_window;

	// Flag indicating the window has been resized. Not used for now
	bool m_framebufferResized;

	// The width of the window
	uint32_t m_width;

	// The height of the window
	uint32_t m_height;

	Device* m_device;

	// the information for the sample is here
	// All of this should be wrapped into proper classes for easy access
	Image* m_depthImage;
	struct {
		Image* colorImage = nullptr;
		Image* normalImage = nullptr;
		Image* depthImage = nullptr;
	} m_GBuffer;
	VkRenderPass m_renderPass;
	VkFramebuffer m_framebuffer;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	UniformBuffer<PerFrameUniformBufferObject>* m_uniformBuffer;
	UniformBuffer<LightInformation>* m_lightUniformBuffer;
	Model* m_model;
	Image* m_modelTexture;
	VkSampler m_sampler;
	VkDescriptorSet m_descriptorSet;
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkSemaphore m_raytracingFinishedSemaphore;
	VkSemaphore m_blitFinishedSemaphore;
	VkFence m_inFlightFence;
	VkFence m_raytracingFence;
	VkFence m_blitFence;
	// need more for double buffering
	VkCommandBuffer m_renderCommandBuffer;
	std::vector<VkCommandBuffer> m_blitCommandBuffers;

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

	float m_cameraAngle;
};

}
