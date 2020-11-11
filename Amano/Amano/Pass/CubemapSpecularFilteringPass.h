#pragma once

#include "../Device.h"

#include <string>

namespace Amano {

class Image;

// This class filters a cubemap to get the specular IBL
// TEMPORARY: to work correctly, it expects the following image states:
//   - inputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT
//   - outputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT
// After submitting, the images states are the same
class CubemapSpecularFilteringPass {
public:
    CubemapSpecularFilteringPass(Device* device);
    virtual ~CubemapSpecularFilteringPass();

    bool init();

    void setupAndRecord(VkCommandBuffer cmd, Image* inputImage, Image* outputImage);

    void clean();

private:
    void recordCommands(VkCommandBuffer cmd, uint32_t size, uint32_t mipCount);
    bool createDescriptorSets(Image* inputImage, Image* outputImage);
    void destroyDescriptorSets();

private:
    Device* m_device;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<VkImageView> m_outputImageViews;
};

}