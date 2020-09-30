#pragma once

#include "Config.h"
#include "Device.h"
#include "Image.h"
#include "Model.h"
#include "UniformBuffer.h"

namespace Amano {

struct PerFrameUniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class Application {
public:
	Application();
	~Application();

	bool init();
	void run();
	void notifyFramebufferResized(int width, int height);

private:
	void initWindow();
	void mainLoop();
	void drawFrame();
	void updateUniformBuffer();

	void recordRenderCommands();
	void recordBlitCommands();

private:
	GLFWwindow* m_window;
	Device* m_device;
	bool m_framebufferResized;
	uint32_t m_width;
	uint32_t m_height;

	// the information for the sample is here
	// All of this should be wrapped into proper classes for easy access
	Image* m_depthImage;
	VkImageView m_depthImageView;
	Image* m_colorImage;
	VkImageView m_colorImageView;
	Image* m_normalImage;
	VkImageView m_normalImageView;
	VkRenderPass m_renderPass;
	VkFramebuffer m_framebuffer;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	UniformBuffer<PerFrameUniformBufferObject>* m_uniformBuffer;
	Model* m_model;
	Image* m_modelTexture;
	VkImageView m_modelTextureView;
	VkSampler m_sampler;
	VkDescriptorSet m_descriptorSet;
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkSemaphore m_blitFinishedSemaphore;
	VkFence m_inFlightFence;
	VkFence m_blitFence;
	// need more for double buffering
	VkCommandBuffer m_renderCommandBuffer;
	std::vector<VkCommandBuffer> m_blitCommandBuffers;
};

}
