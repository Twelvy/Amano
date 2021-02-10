#include "CubemapSpecularFilteringPass.h"
#include "../Image.h"
#include "../Builder/ComputePipelineBuilder.h"
#include "../Builder/DescriptorSetBuilder.h"
#include "../Builder/DescriptorSetLayoutBuilder.h"
#include "../Builder/PipelineLayoutBuilder.h"

#include <vector>

namespace {

std::vector<char> extractLowerCaseCharacters(const std::string& str, bool isReverse) {
    std::vector<char> result;
    auto insert = [&result](char c) {
        if (c >= 'a' && c <= 'z')
            result.push_back(c);
    };
    if (isReverse) {
        for (auto it = str.crbegin(); it != str.crend(); ++it)
            insert(*it);
    }
    else {
        for (auto it = str.cbegin(); it != str.cend(); ++it)
            insert(*it);
    }
    return result;
}

}

namespace Amano {

/////////////////////////////////////////////////////
// CubemapSpecularFilteringPass
/////////////////////////////////////////////////////

CubemapSpecularFilteringPass::CubemapSpecularFilteringPass(Device* device)
    : m_device{ device }
    , m_descriptorSetLayout{ VK_NULL_HANDLE }
    , m_pipelineLayout{ VK_NULL_HANDLE }
    , m_pipeline{ VK_NULL_HANDLE }
    , m_descriptorSets()
    , m_outputImageViews()
{
}

CubemapSpecularFilteringPass::~CubemapSpecularFilteringPass() {
    destroyDescriptorSets();

    vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
}

bool CubemapSpecularFilteringPass::init() {
    DescriptorSetLayoutBuilder descriptorSetLayoutbuilder;
    descriptorSetLayoutbuilder
        .addBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)  // input sampler
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);          // output image
    m_descriptorSetLayout = descriptorSetLayoutbuilder.build(*m_device);

    VkPushConstantRange range;
    range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    range.offset = 0;
    range.size = 4;

    // create pipeline layout
    PipelineLayoutBuilder computePipelineLayoutBuilder;
    computePipelineLayoutBuilder
        .addDescriptorSetLayout(m_descriptorSetLayout)
        .addPushConstantRange(range);
    m_pipelineLayout = computePipelineLayoutBuilder.build(*m_device);

    // load the shaders and create the pipeline
    ComputePipelineBuilder computePipelineBuilder(m_device);
    computePipelineBuilder
        .addShader("compiled_shaders/specular_filter.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    m_pipeline = computePipelineBuilder.build(m_pipelineLayout);

    return true;
}

void CubemapSpecularFilteringPass::setupAndRecord(VkCommandBuffer cmd, Image* inputImage, Image* outputImage) {
    destroyDescriptorSets();
    createDescriptorSets(inputImage, outputImage);
    recordCommands(cmd, outputImage->getWidth(), outputImage->getMipLevels());
}

void CubemapSpecularFilteringPass::clean() {
    destroyDescriptorSets();
}

void CubemapSpecularFilteringPass::recordCommands(VkCommandBuffer cmd, uint32_t size, uint32_t mipCount) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    for (uint32_t i = 0; i < mipCount; ++i) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

        float roughness = static_cast<float>(i) / static_cast<float>(mipCount - 1);
        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &roughness);

        // local size is 32 for x and y
        const uint32_t locaSizeX = 32;
        const uint32_t locaSizeY = 32;
        uint32_t dispatchX = (size + locaSizeX - 1) / locaSizeX;
        uint32_t dispatchY = (size + locaSizeY - 1) / locaSizeY;
        vkCmdDispatch(cmd, dispatchX, dispatchY, 6);

        size >>= 1;
    }
}

bool CubemapSpecularFilteringPass::createDescriptorSets(Image* inputImage, Image* outputImage) {
    uint32_t mipCount = outputImage->getMipLevels();
    m_descriptorSets.reserve(mipCount);
    m_outputImageViews.reserve(mipCount);

    for (uint32_t i = 0; i < mipCount; ++i) {
        VkImageView o = outputImage->createViewHandle(i);
        m_outputImageViews.push_back(o);

        // update the descriptor set
        DescriptorSetBuilder descriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
        descriptorSetBuilder
            .addImage(inputImage->sampler(), inputImage->viewHandle(), 0)
            .addStorageImage(o, 1);
        VkDescriptorSet descriptorSet = descriptorSetBuilder.buildAndUpdate();
        if (descriptorSet == VK_NULL_HANDLE)
            return false;

        m_descriptorSets.push_back(descriptorSet);
    }
    return true;
}

void CubemapSpecularFilteringPass::destroyDescriptorSets() {
    for (uint32_t i = 0; i < m_descriptorSets.size(); ++i)
        vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSets[i]);
    m_descriptorSets.clear();

    for (uint32_t i = 0; i < m_outputImageViews.size(); ++i)
        vkDestroyImageView(m_device->handle(), m_outputImageViews[i], nullptr);
    m_outputImageViews.clear();
}

}