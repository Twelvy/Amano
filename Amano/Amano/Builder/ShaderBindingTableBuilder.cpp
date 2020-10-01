#include "Config.h"
#include "ShaderBindingTableBuilder.h"

#include <array>
#include <fstream>
#include <iostream>

namespace {

uint32_t computeGroupSize(uint32_t inlineSize, uint32_t handleSize, uint32_t alignment)
{
	uint32_t size = handleSize + inlineSize;
	// roundup
	return (size + (alignment - 1)) & ~(alignment - 1);
}

}

namespace Amano {

void ShaderGroupBindingTable::clean(Device* device) {
	if (bufferMemory != VK_NULL_HANDLE)
		device->freeDeviceMemory(bufferMemory);
	if (buffer != VK_NULL_HANDLE)
		device->destroyBuffer(buffer);
}


void ShaderBindingTables::clean(Device* device) {
	rgenShaderBindingTable.clean(device);
	missShaderBindingTable.clean(device);
	chitShaderBindingTable.clean(device);
	//ahitShaderBindingTable.clean(device);
};

ShaderBindingTableBuilder::ShaderBindingTableBuilder(Device* device, VkPipeline pipeline)
	: m_device{ device }
	, m_pipeline{ pipeline }
{
}

ShaderBindingTableBuilder& ShaderBindingTableBuilder::addShader(Stage stage, uint32_t index) {
	switch (stage)
	{
	case Amano::ShaderBindingTableBuilder::Stage::eRayGen:
		setupShaderBindingTable(m_bindingTables.rgenShaderBindingTable, index);
		break;
	case Amano::ShaderBindingTableBuilder::Stage::eMiss:
		setupShaderBindingTable(m_bindingTables.missShaderBindingTable, index);
		break;
	case Amano::ShaderBindingTableBuilder::Stage::eClosestHit:
		setupShaderBindingTable(m_bindingTables.chitShaderBindingTable, index);
		break;
	case Amano::ShaderBindingTableBuilder::Stage::eAnyHit:
		//setupShaderBindingTable(m_bindingTables.ahitShaderBindingTable, index);
		break;
	default:
		break;
	}
	return *this;
}


void ShaderBindingTableBuilder::setupShaderBindingTable(ShaderGroupBindingTable& table, uint32_t index) {
	VkPhysicalDeviceRayTracingPropertiesNV physicalProperties = m_device->getRaytracingPhysicalProperties();

	// TODO: round up to alignment
	// allocate memory for all the shaders
	// 0 for inline data
	table.groupSize = computeGroupSize(0, physicalProperties.shaderGroupHandleSize, physicalProperties.shaderGroupBaseAlignment);

	m_device->createBufferAndMemory(
		table.groupSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		table.buffer,
		table.bufferMemory);

	// allocate a buffer as big as the number of shaders
	// TODO: remove the hardcoded 3
	std::vector<uint8_t> shaderHandleStorage(physicalProperties.shaderGroupHandleSize);

	if (m_device->getExtensions().vkGetRayTracingShaderGroupHandlesNV(m_device->handle(), m_pipeline, index, 1, shaderHandleStorage.size(), shaderHandleStorage.data()) != VK_SUCCESS) {
		std::cerr << "Cannot get shader group handles" << std::endl;
	}

	void* tmpData;
	vkMapMemory(m_device->handle(), table.bufferMemory, 0, table.groupSize, 0, &tmpData);
	uint8_t* pData = static_cast<uint8_t*>(tmpData);
	// copy the handles
	memset(pData, 0, table.groupSize);
	memcpy(pData, shaderHandleStorage.data(), physicalProperties.shaderGroupHandleSize);
}


ShaderBindingTables ShaderBindingTableBuilder::build() {
	// TODO: use a single buffer?
	return m_bindingTables;
}

}
