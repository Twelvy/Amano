#include "IBLLutPass.h"
#include "../Image.h"
#include "../Builder/ComputePipelineBuilder.h"
#include "../Builder/DescriptorSetBuilder.h"
#include "../Builder/DescriptorSetLayoutBuilder.h"
#include "../Builder/PipelineLayoutBuilder.h"

namespace Amano {

IBLLutPass::IBLLutPass(Device* device)
    : m_device{ device }
    , m_descriptorSetLayout{ VK_NULL_HANDLE }
    , m_pipelineLayout{ VK_NULL_HANDLE }
    , m_pipeline{ VK_NULL_HANDLE }
    , m_descriptorSet{ VK_NULL_HANDLE }
{
}

IBLLutPass::~IBLLutPass() {
    destroyDescriptorSet();

    vkDestroyDescriptorSetLayout(m_device->handle(), m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
}

bool IBLLutPass::init() {
    DescriptorSetLayoutBuilder descriptorSetLayoutbuilder;
    descriptorSetLayoutbuilder
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);          // output image
    m_descriptorSetLayout = descriptorSetLayoutbuilder.build(*m_device);

    // create raytracing pipeline layout
    PipelineLayoutBuilder computePipelineLayoutBuilder;
    computePipelineLayoutBuilder.addDescriptorSetLayout(m_descriptorSetLayout);
    m_pipelineLayout = computePipelineLayoutBuilder.build(*m_device);

    // load the shaders and create the pipeline
    ComputePipelineBuilder computePipelineBuilder(m_device);
    computePipelineBuilder
        .addShader("compiled_shaders/ibl_lut.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
    m_pipeline = computePipelineBuilder.build(m_pipelineLayout);

    return true;
}

void IBLLutPass::setupAndRecord(VkCommandBuffer cmd, Image* outputImage) {
    destroyDescriptorSet();
    createDescriptorSet(outputImage);
    recordCommands(cmd, outputImage->getWidth(), outputImage->getHeight());
}

void IBLLutPass::clean() {
    destroyDescriptorSet();
}

void IBLLutPass::recordCommands(VkCommandBuffer cmd, uint32_t width, uint32_t height) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

    // local size is 32 for x and y
    const uint32_t locaSizeX = 32;
    const uint32_t locaSizeY = 32;
    uint32_t dispatchX = (width + locaSizeX - 1) / locaSizeX;
    uint32_t dispatchY = (height + locaSizeY - 1) / locaSizeY;
    vkCmdDispatch(cmd, dispatchX, dispatchY, 6);
}

bool IBLLutPass::createDescriptorSet(Image* outputImage) {
    // update the descriptor set
    DescriptorSetBuilder descriptorSetBuilder(m_device, 2, m_descriptorSetLayout);
    descriptorSetBuilder
        .addStorageImage(outputImage->viewHandle(), 0);
    m_descriptorSet = descriptorSetBuilder.buildAndUpdate();

    return m_descriptorSet != VK_NULL_HANDLE;
}

void IBLLutPass::destroyDescriptorSet() {
    if (m_descriptorSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_device->handle(), m_device->getDescriptorPool(), 1, &m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }
}

}