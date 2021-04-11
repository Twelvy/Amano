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
	VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationProperties = m_device->getPhysicalAccelerationStructureProperties();
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties = m_device->getPhysicalRaytracingPipelineProperties();

	// TODO: round up to alignment
	// allocate memory for all the shaders
	// 0 for inline data
	table.groupSize = computeGroupSize(0, pipelineProperties.shaderGroupHandleSize, pipelineProperties.shaderGroupBaseAlignment);

	// OTE: we could also allocate once for all the programs and use offsets
	// to do so, we should calculate the max size of each group, allocate this size for each program in the group
	// then sum all the sizes for all the groups to get the final size
	// R raygen, M miss, H hits
	// total size = maxGroupSize(R) * R + maxGroupSize(M) * M + maxGroupSize(H) * H
	// offset(R) = 0, offset(M) = maxGroupSize(R) * R, offset(H) = offset(M) + maxGroupSize(M) * M


	m_device->createBufferAndMemory(
		table.groupSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		table.buffer,
		table.bufferMemory);

	// allocate a buffer as big as the number of shaders
	// shaderHandleStorage should have the size of R + M + H
	std::vector<uint8_t> shaderHandleStorage(pipelineProperties.shaderGroupHandleSize);

	if (m_device->getExtensions().vkGetRayTracingShaderGroupHandlesKHR(
		m_device->handle(),
		m_pipeline,
		index,
		1, // group count so R + M + H
		shaderHandleStorage.size(),
		shaderHandleStorage.data()) != VK_SUCCESS) {
		std::cerr << "Cannot get shader group handles" << std::endl;
	}
	
	// Copy the shader identifiers followed by their resource pointers or root constants: 
	void* tmpData;
	vkMapMemory(m_device->handle(), table.bufferMemory, 0, table.groupSize, 0, &tmpData);
	uint8_t* pData = static_cast<uint8_t*>(tmpData);
	// copy the handles
	memset(pData, 0, table.groupSize);
	memcpy(pData, shaderHandleStorage.data(), pipelineProperties.shaderGroupHandleSize);
	vkUnmapMemory(m_device->handle(), table.bufferMemory);
}


ShaderBindingTables ShaderBindingTableBuilder::build() {
	// TODO: use a single buffer?
	return m_bindingTables;
}

}
