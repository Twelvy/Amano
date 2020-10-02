#include "PipelineBuilderBase.h"

#include <array>
#include <fstream>
#include <iostream>

namespace {
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			std::cerr << "failed to open file!" << std::endl;
			std::vector<char> buffer;
			return buffer;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
}

namespace Amano {

PipelineBuilderBase::PipelineBuilderBase(Device* device)
	: m_device{ device }
	, m_shaderModules()
	, m_shaderStages()
{
}

PipelineBuilderBase::~PipelineBuilderBase() {
	for (auto& module : m_shaderModules) {
		vkDestroyShaderModule(m_device->handle(), module, nullptr);
	}
}

void PipelineBuilderBase::addShaderBase(const std::string& filename, VkShaderStageFlagBits stage) {
	auto shaderCode = readFile(filename);

	VkShaderModule shaderModule = createShaderModule(shaderCode);
	m_shaderModules.push_back(shaderModule);

	auto& shaderStageInfo = m_shaderStages.emplace_back();
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext = nullptr;
	shaderStageInfo.stage = stage;
	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main"; // this is the convention for now
}


VkShaderModule PipelineBuilderBase::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	if (vkCreateShaderModule(m_device->handle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		std::cerr << "failed to create shader module!" << std::endl;
	}

	return shaderModule;
}

}
