#pragma once

#include "../Config.h"
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

	// instancing isn't supported for now
	// transformation isn't supported either
	RaytracingAccelerationStructureBuilder& addGeometry(Mesh& mesh);

	AccelerationStructures build();

private:
	bool createBottomLevelAccelerationStructure(VkCommandBuffer cmd);
	bool createTopLevelAccelerationStructure(VkCommandBuffer cmd);

private:
	Device* m_device;
	VkPipeline m_pipeline;
	std::vector<VkGeometryNV> m_geometries;
	AccelerationStructures m_accelerationStructures;
};

}
