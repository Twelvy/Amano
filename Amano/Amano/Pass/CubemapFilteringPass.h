#pragma once

#include "../Device.h"

#include <string>

namespace Amano {

class Image;

// This class filters a cubemap
// TEMPORARY: to work correctly, it expects the following image states:
//   - inputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT
//   - outputImage: VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT
// After submitting, the images states are the same
class CubemapFilteringPass {
public:
    void setupAndRecord(VkCommandBuffer cmd, Image* inputImage, Image* outputImage);

    void clean();

protected:
    CubemapFilteringPass(Device* device);
    virtual ~CubemapFilteringPass();

    bool init(const std::string& filename);

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

// This class filters a cubemap to get the diffuse IBL
class CubemapDiffuseFilteringPass : public CubemapFilteringPass {
public:
    CubemapDiffuseFilteringPass(Device* device);
    ~CubemapDiffuseFilteringPass() {}
    bool init();
};

}