#pragma once

#include "../Device.h"
#include "../Mesh.h"

#include <vector>

namespace Amano {

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

	void clean(Device* device);
};


class RaytracingAccelerationStructureBuilder {
public:
	RaytracingAccelerationStructureBuilder(Device* device, VkPipeline pipeline);

	AccelerationStructures build(Mesh& mesh);

private:
	bool createBottomLevelAccelerationStructure(VkCommandBuffer cmd, Mesh& mesh);
	bool createTopLevelAccelerationStructure(VkCommandBuffer cmd);

private:
	Device* m_device;
	VkPipeline m_pipeline;
	AccelerationStructures m_accelerationStructures;
};

}
