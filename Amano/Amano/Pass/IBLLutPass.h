#pragma once

#include "../Device.h"

#include <string>

namespace Amano {

class Image;

// This class creates the IBL Lut texture
// TEMPORARY: to work correctly, it expects the following image states:
//   - outputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT
// After submitting, the images states are the same
class IBLLutPass {
public:
    IBLLutPass(Device* device);
    virtual ~IBLLutPass();

    bool init();
    void setupAndRecord(VkCommandBuffer cmd, Image* outputImage);
    void clean();

private:
    void recordCommands(VkCommandBuffer cmd, uint32_t width, uint32_t height);
    bool createDescriptorSet(Image* outputImage);
    void destroyDescriptorSet();

private:
    Device* m_device;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    VkDescriptorSet m_descriptorSet;
};

}