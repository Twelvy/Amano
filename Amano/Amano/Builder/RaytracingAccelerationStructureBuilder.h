#pragma once

#include "../Config.h"
#include "../Device.h"
#include "../Model.h"

#include <vector>

namespace Amano {

struct RaytracingBuffers {
	VkBuffer result = VK_NULL_HANDLE;
	VkDeviceMemory resultMemory = VK_NULL_HANDLE;
	VkBuffer build = VK_NULL_HANDLE;
	VkDeviceMemory buildMemory = VK_NULL_HANDLE;
	VkBuffer update = VK_NULL_HANDLE;
	VkDeviceMemory updateMemory = VK_NULL_HANDLE;

	void clean(Device* device);
};

struct ShaderBindingTable {
	RaytracingBuffers bottom;
	RaytracingBuffers top;
	VkBuffer topInstanceBuffer = VK_NULL_HANDLE;
	VkDeviceMemory topInstanceBufferMemory = VK_NULL_HANDLE;
	VkBuffer shaderBindingTableBuffer = VK_NULL_HANDLE;
	VkDeviceMemory shaderBindingTableBufferMemory = VK_NULL_HANDLE;

	void clean(Device* device);
};

struct AccelerationStructures {
	VkAccelerationStructureKHR top;
	VkAccelerationStructureKHR bottom;
	ShaderBindingTable bindingTable;

	struct {
		uint32_t raygen;
		uint32_t miss;
		uint32_t closestHit;
		uint32_t anyHit;

		uint32_t rgenGroupSize;
		uint32_t missGroupSize;
		uint32_t chitGroupSize;

		uint32_t rgenGroupOffset;
		uint32_t missGroupOffset;
		uint32_t chitGroupOffset;
	} raytracingShaderGroupIndices;

	void clean(Device* device);
};

class RaytracingAccelerationStructureBuilder {
public:
	RaytracingAccelerationStructureBuilder(Device* device);

	RaytracingAccelerationStructureBuilder& addRayGenShader(uint32_t index);
	RaytracingAccelerationStructureBuilder& addMissShader(uint32_t index);
	RaytracingAccelerationStructureBuilder& addClosestHitShader(uint32_t index);

	AccelerationStructures build(Model& model, VkPipeline pipeline);

private:
	bool createBottomLevelAccelerationStructure(VkCommandBuffer cmd, Model& model);
	bool createTopLevelAccelerationStructure(VkCommandBuffer cmd);

private:
	Device* m_device;
	AccelerationStructures m_accelerationStructures;
};

}
