#include "RaytracingAccelerationStructureBuilder.h"

#include <iostream>

namespace {

struct AccelerationStructures{
	VkAccelerationStructureKHR top;
	VkAccelerationStructureKHR bottom;
};

uint64_t RoundUp(uint64_t size, uint64_t alignment) {
	const uint64_t t = (size + alignment - 1) / alignment;
	return t * alignment;
}

// TODO: wrap device memory into a small class to perform those calls
VkDeviceAddress GetDeviceAddress(Amano::Device* device, VkBuffer buffer) {
	VkBufferDeviceAddressInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	info.pNext = nullptr;
	info.buffer = buffer;

	return vkGetBufferDeviceAddress(device->handle(), &info);
}

}

namespace Amano {

void AccelerationStructureInfo::clean(Device* device) {
	device->getExtensions().vkDestroyAccelerationStructureKHR(device->handle(), handle, nullptr);
	device->freeDeviceMemory(resultMemory);
	device->freeDeviceMemory(scratchMemory);
	device->destroyBuffer(result);
	device->destroyBuffer(scratch);

	if (instanceMemory != VK_NULL_HANDLE)
		device->freeDeviceMemory(instanceMemory);
	if (instance != VK_NULL_HANDLE)
		device->destroyBuffer(instance);
}

void AccelerationStructures::clean(Device* device) {
	top.clean(device);
	bottom.clean(device);
};


RaytracingAccelerationStructureBuilder::RaytracingAccelerationStructureBuilder(Device* device, VkPipeline pipeline)
	: m_device{ device }
	, m_pipeline{ pipeline }
	, m_geometries()
	, m_buildRangeInfo()
	, m_accelerationStructures{}
{
}

RaytracingAccelerationStructureBuilder& RaytracingAccelerationStructureBuilder::addGeometry(Mesh& mesh) {
	auto& geometry = m_geometries.emplace_back();
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = nullptr;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.pNext = nullptr;
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.vertexData.deviceAddress = mesh.getVertexBufferAddress();
	geometry.geometry.triangles.vertexData.hostAddress = nullptr;
	geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	geometry.geometry.triangles.maxVertex = mesh.getVertexCount();
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.indexData.deviceAddress = mesh.getIndexBufferAddress();
	geometry.geometry.triangles.indexData.hostAddress = nullptr;
	geometry.geometry.triangles.transformData = {};

	geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
	geometry.geometry.aabbs.pNext = nullptr;
	// NOTE: should AABB be set manually?

	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.pNext = nullptr;
	// NOTE: should instances be set manually?

	auto& buildRangeInfo = m_buildRangeInfo.emplace_back();
	buildRangeInfo.firstVertex = 0;  // no offset
	buildRangeInfo.primitiveOffset = 0;  // no offset
	buildRangeInfo.primitiveCount = mesh.getIndexCount() / 3;
	buildRangeInfo.transformOffset = 0;

	return *this;
}


VkAccelerationStructureBuildSizesInfoKHR RaytracingAccelerationStructureBuilder::getBuildSizesInfo(VkAccelerationStructureKHR accStructure, const uint32_t* pMaxPrimitiveCounts, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildGeomtryInfo) {
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	sizeInfo.pNext = nullptr;

	m_device->getExtensions().vkGetAccelerationStructureBuildSizesKHR(
		m_device->handle(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		pBuildGeomtryInfo,
		pMaxPrimitiveCounts,
		&sizeInfo);

	// AccelerationStructure offset needs to be 256 bytes aligned
	// official Vulkan specs, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#acceleration-structure-def
	const uint64_t cAccelerationStructureAlignment = 256;
	const uint64_t cScratchAlignment = m_device->getPhysicalAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;

	sizeInfo.accelerationStructureSize = RoundUp(sizeInfo.accelerationStructureSize, cAccelerationStructureAlignment);
	sizeInfo.buildScratchSize = RoundUp(sizeInfo.buildScratchSize, cScratchAlignment);

	return sizeInfo;
}

bool RaytracingAccelerationStructureBuilder::createBottomLevelAccelerationStructure(VkCommandBuffer cmd) {
	// we can create one bottom acceleration structure per model/geometry, or group them into 1 bottom structure

	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
	buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildGeometryInfo.pNext = nullptr;
	buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // TODO: check that
	buildGeometryInfo.geometryCount = static_cast<uint32_t>(m_geometries.size());
	buildGeometryInfo.pGeometries = m_geometries.data();
	buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;

	std::vector<uint32_t> maxPrimitiveCounts(m_buildRangeInfo.size());

	for (size_t i = 0; i < maxPrimitiveCounts.size(); ++i)
		maxPrimitiveCounts[i] = m_buildRangeInfo[i].primitiveCount;

	VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = getBuildSizesInfo(m_accelerationStructures.bottom.handle, maxPrimitiveCounts.data(), &buildGeometryInfo);

	// allocate the memory
	// if we had multiple bottom acceleration structure, we woud sum all the sizes and allocate a big chunk of memory for them all, then use offsets
	// this would require some refactoring here since we only support 1 bottom acceleration structure

	m_device->createBufferAndMemory(
		buildSizesInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.bottom.result,
		m_accelerationStructures.bottom.resultMemory);

	m_device->createBufferAndMemory(
		buildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.bottom.scratch,
		m_accelerationStructures.bottom.scratchMemory);


	VkAccelerationStructureCreateInfoKHR bottomAccelerationInfo{};
	bottomAccelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	bottomAccelerationInfo.pNext = nullptr;
	bottomAccelerationInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	bottomAccelerationInfo.size = buildSizesInfo.accelerationStructureSize;
	bottomAccelerationInfo.buffer = m_accelerationStructures.bottom.result;
	bottomAccelerationInfo.offset = 0;  // i * buildSizesInfo.accelerationStructureSize if we had multiple bottom acceleration structures
	// bottomAccelerationInfo.deviceAddress isn't used

	if (m_device->getExtensions().vkCreateAccelerationStructureKHR(m_device->handle(), &bottomAccelerationInfo, nullptr, &m_accelerationStructures.bottom.handle) != VK_SUCCESS) {
		std::cerr << "failed to create bottom acceleration structure!" << std::endl;
		return false;
	}

	const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = m_buildRangeInfo.data();
	buildGeometryInfo.dstAccelerationStructure = m_accelerationStructures.bottom.handle;
	buildGeometryInfo.scratchData.deviceAddress = GetDeviceAddress(m_device, m_accelerationStructures.bottom.scratch);
	
	m_device->getExtensions().vkCmdBuildAccelerationStructuresKHR(
		cmd,
		1,
		&buildGeometryInfo,
		&pBuildRangeInfo);

	return true;
}

bool RaytracingAccelerationStructureBuilder::createTopLevelAccelerationStructure(VkCommandBuffer cmd) {
	// TODO: only one instance of per geometry is supported for now. Change that afterwards
	std::vector<VkAccelerationStructureInstanceKHR> instances(m_geometries.size());
	for (uint32_t instanceId = 0; instanceId < m_geometries.size(); ++instanceId) {
		auto& instance = instances[instanceId];

		VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
		addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure = m_accelerationStructures.bottom.handle;  // should get the associated bottom AS, we only have one for now

		const VkDeviceAddress address = m_device->getExtensions().vkGetAccelerationStructureDeviceAddressKHR(m_device->handle(), &addressInfo);

		glm::mat4 transform = glm::mat4(1); // TODO: support transformation - should we transpose it?
		// The instance.transform value only contains 12 values, corresponding to a 4x3 matrix,
		// hence saving the last row that is anyway always (0,0,0,1).
		// Since the matrix is row-major, we simply copy the first 12 values of the original 4x4 matrix
		std::memcpy(&instance.transform, &transform, sizeof(transform));
		instance.instanceCustomIndex = instanceId;
		instance.mask = 0xFF; // The visibility mask is always set of 0xFF, but if some instances would need to be ignored in some cases, this flag should be passed by the application.
		instance.instanceShaderBindingTableRecordOffset = 0; // Set the hit group index, that will be used to find the shader code to execute when hitting the geometry.
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; // Disable culling - more fine control could be provided by the application
		instance.accelerationStructureReference = address;
	}

	// create the buffer for instances
	size_t instancesSizes = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
	m_device->createBufferAndMemory(
		instancesSizes,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.top.instance,
		m_accelerationStructures.top.instanceMemory);

	// copy from staging buffer
	{
		// create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_device->createBufferAndMemory(
			instancesSizes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// copy the data into it
		void* data;
		vkMapMemory(m_device->handle(), stagingBufferMemory, 0, instancesSizes, 0, &data);
		memcpy(data, instances.data(), (size_t)instancesSizes);
		vkUnmapMemory(m_device->handle(), stagingBufferMemory);

		// copy command
		m_device->copyBuffer(stagingBuffer, m_accelerationStructures.top.instance, instancesSizes, QueueType::eGraphics);

		// delete buffer and memory
		// TODO: destroy buffer then memory?
		m_device->destroyBuffer(stagingBuffer);
		m_device->freeDeviceMemory(stagingBufferMemory);
	}

	// Wait for the builder to complete by setting a barrier on the resulting buffer. This is
	// particularly important as the construction of the top-level hierarchy may be called right
	// afterwards, before executing the command list.
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	
	VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
	instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instancesData.pNext = nullptr;
	instancesData.arrayOfPointers = VK_FALSE;
	instancesData.data.deviceAddress = GetDeviceAddress(m_device, m_accelerationStructures.top.instance);
	instancesData.data.hostAddress = nullptr;

	VkAccelerationStructureGeometryKHR topAccelerationStructureGeometry{};
	topAccelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	topAccelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topAccelerationStructureGeometry.geometry.instances = instancesData;

	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
	buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildGeometryInfo.pNext = nullptr;
	buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildGeometryInfo.geometryCount = 1;
	buildGeometryInfo.pGeometries = &topAccelerationStructureGeometry;
	buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;

	uint32_t instancesCount = static_cast<uint32_t>(instances.size());
	VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = getBuildSizesInfo(m_accelerationStructures.top.handle, &instancesCount, &buildGeometryInfo);

	m_device->createBufferAndMemory(
		buildSizesInfo.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.top.result,
		m_accelerationStructures.top.resultMemory);

	m_device->createBufferAndMemory(
		buildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.top.scratch,
		m_accelerationStructures.top.scratchMemory);

	// create the acceleration structure
	VkAccelerationStructureCreateInfoKHR topAccelerationInfo{};
	topAccelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	topAccelerationInfo.pNext = nullptr;
	topAccelerationInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	topAccelerationInfo.size = buildSizesInfo.accelerationStructureSize;
	topAccelerationInfo.buffer = m_accelerationStructures.top.result;
	topAccelerationInfo.offset = 0;  // i * buildSizesInfo.accelerationStructureSize if we had multiple bottom acceleration structures
	// bottomAccelerationInfo.deviceAddress isn't used

	if (m_device->getExtensions().vkCreateAccelerationStructureKHR(m_device->handle(), &topAccelerationInfo, nullptr, &m_accelerationStructures.top.handle) != VK_SUCCESS) {
		std::cerr << "failed to create top acceleration structure!" << std::endl;
		return false;
	}

	// Build the actual bottom-level acceleration structure
	VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
	buildOffsetInfo.primitiveCount = instancesCount;

	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

	buildGeometryInfo.dstAccelerationStructure = m_accelerationStructures.top.handle;
	buildGeometryInfo.scratchData.deviceAddress = GetDeviceAddress(m_device, m_accelerationStructures.top.scratch);

	m_device->getExtensions().vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildGeometryInfo, &pBuildOffsetInfo);

	return true;
}

AccelerationStructures RaytracingAccelerationStructureBuilder::build() {
	Queue* pQueue = m_device->getQueue(QueueType::eGraphics);
	VkCommandBuffer cmd = pQueue->beginSingleTimeCommands();

	createBottomLevelAccelerationStructure(cmd);
	
	// Wait for the builder to complete by setting a barrier on the resulting buffer. This is
		// particularly important as the construction of the top-level hierarchy may be called right
		// afterwards, before executing the command list.
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	createTopLevelAccelerationStructure(cmd);

	pQueue->endSingleTimeCommands(cmd);

	return m_accelerationStructures;
}

}
