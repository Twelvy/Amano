#pragma once
#pragma once

#include "../Device.h"
#include "../Ubo.h"
#include "../UniformBuffer.h"

namespace Amano {

class Image;

// This class rotates the cubemap from an indirect referential with Y up to a direct one with Z up
// TEMPORARY: to work correctly, it expects the following image states:
//   - inputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT
//   - outputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT
// After submitting, the images states are the same
class CubemapRotatePass {
public:
    CubemapRotatePass(Device* device);
    ~CubemapRotatePass();

    bool init();
    void setupAndRecord(VkCommandBuffer cmd, Image* inputImage, Image* outputImage);

    void clean();

private:
    void recordCommands(VkCommandBuffer cmd, uint32_t width, uint32_t height);
    bool createDescriptorSet(Image* inputImage, Image* outputImage);
    void destroyDescriptorSet();

private:
    Device* m_device;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    VkDescriptorSet m_descriptorSet;
};

}