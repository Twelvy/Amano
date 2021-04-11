#pragma once

#include "../Device.h"
#include "../Mesh.h"

#include <vector>

namespace Amano {

struct AccelerationStructureInfo {
	VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
	VkBuffer result = VK_NULL_HANDLE;
	VkDeviceMemory resultMemory = VK_NULL_HANDLE;
	VkBuffer scratch = VK_NULL_HANDLE;
	VkDeviceMemory scratchMemory = VK_NULL_HANDLE;

	// only for top
	VkBuffer instance = VK_NULL_HANDLE;
	VkDeviceMemory instanceMemory = VK_NULL_HANDLE;

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
	VkAccelerationStructureBuildSizesInfoKHR getBuildSizesInfo(VkAccelerationStructureKHR accStructure, const uint32_t* pMaxPrimitiveCounts, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildGeomtryInfo);
	bool createBottomLevelAccelerationStructure(VkCommandBuffer cmd);
	bool createTopLevelAccelerationStructure(VkCommandBuffer cmd);

private:
	Device* m_device;
	VkPipeline m_pipeline;
	std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_buildRangeInfo;
	AccelerationStructures m_accelerationStructures;
};

}
