#pragma once

#include "../Config.h"
#include "../Device.h"
#include "../Model.h"

#include <vector>

namespace Amano {

struct ShaderBingTable {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
	uint32_t groupSize = 0;

	void clean(Device* device);
};

struct AccelerationStructureInfo {
	VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
	VkBuffer result = VK_NULL_HANDLE;
	VkDeviceMemory resultMemory = VK_NULL_HANDLE;
	VkBuffer build = VK_NULL_HANDLE;
	VkDeviceMemory buildMemory = VK_NULL_HANDLE;

	// only for top
	VkBuffer topInstance = VK_NULL_HANDLE;
	VkDeviceMemory topInstanceMemory = VK_NULL_HANDLE;

	void clean(Device* device);
};

struct AccelerationStructures {
	AccelerationStructureInfo top;
	AccelerationStructureInfo bottom;
	ShaderBingTable rgenShaderBindingTable;
	ShaderBingTable missShaderBindingTable;
	ShaderBingTable chitShaderBindingTable;
	//ShaderBingTable ahitShaderBindingTable;

	void clean(Device* device);
};


class RaytracingAccelerationStructureBuilder {
public:
	RaytracingAccelerationStructureBuilder(Device* device, VkPipeline pipeline);

	RaytracingAccelerationStructureBuilder& addRayGenShader(uint32_t index);
	RaytracingAccelerationStructureBuilder& addMissShader(uint32_t index);
	RaytracingAccelerationStructureBuilder& addClosestHitShader(uint32_t index);

	AccelerationStructures build(Model& model);

private:
	void setupShaderBindingTable(ShaderBingTable& table, uint32_t index);
	bool createBottomLevelAccelerationStructure(VkCommandBuffer cmd, Model& model);
	bool createTopLevelAccelerationStructure(VkCommandBuffer cmd);

private:
	Device* m_device;
	VkPipeline m_pipeline;
	AccelerationStructures m_accelerationStructures;
};

}
