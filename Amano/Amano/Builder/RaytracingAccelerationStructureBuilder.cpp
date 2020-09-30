#include "RaytracingAccelerationStructureBuilder.h"

#include <iostream>

namespace {

struct AccelerationStructureMemoryRequirements {
	VkMemoryRequirements result;
	VkMemoryRequirements build;
	VkMemoryRequirements update;
};

// Geometry instance, with the layout expected by VK_NV_ray_tracing
struct VkGeometryInstance
{
	/// Transform matrix, containing only the top 3 rows
	float transform[12];
	/// Instance index
	uint32_t instanceCustomIndex : 24;
	/// Visibility mask
	uint32_t mask : 8;
	/// Index of the hit group which will be invoked when a ray hits the instance
	uint32_t instanceOffset : 24;
	/// Instance flags, such as culling
	uint32_t flags : 8;
	/// Opaque handle of the bottom-level acceleration structure
	uint64_t accelerationStructureHandle;
};

struct AccelerationStructures{
	VkAccelerationStructureKHR top;
	VkAccelerationStructureKHR bottom;
};

AccelerationStructureMemoryRequirements getMemoryRequirements(Amano::Device* device, VkAccelerationStructureNV accelerationStructure) {
	VkAccelerationStructureMemoryRequirementsInfoNV accMemoryRequirements{};
	accMemoryRequirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	accMemoryRequirements.pNext = nullptr;
	accMemoryRequirements.accelerationStructure = accelerationStructure;

	AccelerationStructureMemoryRequirements req;

	VkMemoryRequirements2KHR memoryRequirements{};

	accMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	device->getExtensions().vkGetAccelerationStructureMemoryRequirementsNV(device->handle(), &accMemoryRequirements, &memoryRequirements);
	req.result = memoryRequirements.memoryRequirements;

	accMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	device->getExtensions().vkGetAccelerationStructureMemoryRequirementsNV(device->handle(), &accMemoryRequirements, &memoryRequirements);
	req.build = memoryRequirements.memoryRequirements;

	accMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
	device->getExtensions().vkGetAccelerationStructureMemoryRequirementsNV(device->handle(), &accMemoryRequirements, &memoryRequirements);
	req.update = memoryRequirements.memoryRequirements;

	return req;
}

}

namespace Amano {

void AccelerationStructureInfo::clean(Device* device) {
	device->getExtensions().vkDestroyAccelerationStructureNV(device->handle(), handle, nullptr);
	device->freeDeviceMemory(resultMemory);
	device->freeDeviceMemory(buildMemory);
	device->destroyBuffer(result);
	device->destroyBuffer(build);

	if (topInstanceMemory != VK_NULL_HANDLE)
		device->freeDeviceMemory(topInstanceMemory);
	if (topInstance != VK_NULL_HANDLE)
		device->destroyBuffer(topInstance);
}

void AccelerationStructures::clean(Device* device) {
	top.clean(device);
	bottom.clean(device);
};


RaytracingAccelerationStructureBuilder::RaytracingAccelerationStructureBuilder(Device* device, VkPipeline pipeline)
	: m_device{ device }
	, m_pipeline{ pipeline }
	, m_accelerationStructures{}
{
}

bool RaytracingAccelerationStructureBuilder::createBottomLevelAccelerationStructure(VkCommandBuffer cmd, Model& model) {
	// we should create one bottom acceleration structure per mesh

	// geometries
	VkGeometryNV geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geometry.pNext = nullptr;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geometry.geometry.triangles.pNext = nullptr;
	geometry.geometry.triangles.vertexData = model.getVertexBuffer();
	geometry.geometry.triangles.vertexCount = model.getVertexCount();
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	geometry.geometry.triangles.indexData = model.getIndexBuffer();
	geometry.geometry.triangles.indexCount = model.getIndexCount();
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.indexOffset = 0;
	geometry.geometry.triangles.transformData = nullptr;
	geometry.geometry.triangles.transformOffset = 0;
	geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
	geometry.geometry.aabbs.pNext = nullptr;

	VkAccelerationStructureCreateInfoNV bottomAccelerationInfo{};
	bottomAccelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	bottomAccelerationInfo.pNext = nullptr;
	bottomAccelerationInfo.compactedSize = 0;
	bottomAccelerationInfo.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	bottomAccelerationInfo.info.pNext = nullptr;
	bottomAccelerationInfo.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV; // VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV
	bottomAccelerationInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	bottomAccelerationInfo.info.geometryCount = 1;
	bottomAccelerationInfo.info.instanceCount = 0;
	bottomAccelerationInfo.info.pGeometries = &geometry;

	if (m_device->getExtensions().vkCreateAccelerationStructureNV(m_device->handle(), &bottomAccelerationInfo, nullptr, &m_accelerationStructures.bottom.handle) != VK_SUCCESS) {
		std::cerr << "failed to create bottom acceleration structure!" << std::endl;
		return false;
	}

	auto requirements = getMemoryRequirements(m_device, m_accelerationStructures.bottom.handle);

	m_device->createBufferAndMemory(
		requirements.result.size,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.bottom.result,
		m_accelerationStructures.bottom.resultMemory);
	m_device->createBufferAndMemory(
		requirements.build.size,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.bottom.build,
		m_accelerationStructures.bottom.buildMemory);
	
	VkBindAccelerationStructureMemoryInfoNV bindInfo = {};
	bindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	bindInfo.pNext = nullptr;
	bindInfo.accelerationStructure = m_accelerationStructures.bottom.handle;
	bindInfo.memory = m_accelerationStructures.bottom.resultMemory;
	bindInfo.memoryOffset = 0;
	bindInfo.deviceIndexCount = 0;
	bindInfo.pDeviceIndices = nullptr;

	if (m_device->getExtensions().vkBindAccelerationStructureMemoryNV(m_device->handle(), 1, &bindInfo) != VK_SUCCESS) {
		std::cerr << "failed to create top acceleration structure!" << std::endl;
		return false;
	}

	VkAccelerationStructureInfoNV buildInfo = {};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.pNext = nullptr;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV; //VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.instanceCount = 0;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry;

	m_device->getExtensions().vkCmdBuildAccelerationStructureNV(
		cmd,
		&buildInfo,
		nullptr,
		0,
		false,
		m_accelerationStructures.bottom.handle,
		nullptr,
		m_accelerationStructures.bottom.build,
		0);

	return true;
}

bool RaytracingAccelerationStructureBuilder::createTopLevelAccelerationStructure(VkCommandBuffer cmd) {
	VkAccelerationStructureCreateInfoNV topAccelerationInfo{};
	topAccelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	topAccelerationInfo.pNext = nullptr;
	topAccelerationInfo.compactedSize = 0;
	topAccelerationInfo.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	topAccelerationInfo.info.pNext = nullptr;
	topAccelerationInfo.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV; // VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV
	topAccelerationInfo.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	topAccelerationInfo.info.geometryCount = 0;
	topAccelerationInfo.info.instanceCount = 1;
	topAccelerationInfo.info.pGeometries = nullptr;

	if (m_device->getExtensions().vkCreateAccelerationStructureNV(m_device->handle(), &topAccelerationInfo, nullptr, &m_accelerationStructures.top.handle) != VK_SUCCESS) {
		std::cerr << "failed to create top acceleration structure!" << std::endl;
		return false;
	}

	uint64_t handle = 0;
	if (m_device->getExtensions().vkGetAccelerationStructureHandleNV(m_device->handle(), m_accelerationStructures.bottom.handle, sizeof(uint64_t), &handle) != VK_SUCCESS) {
		std::cerr << "failed to get bottom acceleration structure handle!" << std::endl;
		return false;
	}

	uint32_t instanceId = 0;
	VkGeometryInstance geometryInstance = {};
	glm::mat4 transform = glm::mat4(1); // no transformation
	std::memcpy(geometryInstance.transform, &transform, sizeof(glm::mat4));
	geometryInstance.instanceCustomIndex = instanceId;
	geometryInstance.mask = 0xFF; // The visibility mask is always set of 0xFF, but if some instances would need to be ignored in some cases, this flag should be passed by the application.
	geometryInstance.instanceOffset = 0; // Set the hit group index, that will be used to find the shader code to execute when hitting the geometry.
	geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV; // Disable culling - more fine control could be provided by the application
	geometryInstance.accelerationStructureHandle = handle;

	auto requirements = getMemoryRequirements(m_device, m_accelerationStructures.top.handle);

	m_device->createBufferAndMemory(
		requirements.result.size,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.top.result,
		m_accelerationStructures.top.resultMemory);
	m_device->createBufferAndMemory(
		requirements.build.size,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_accelerationStructures.top.build,
		m_accelerationStructures.top.buildMemory);
	m_device->createBufferAndMemory(
		sizeof(VkGeometryInstance) * 1,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_accelerationStructures.top.topInstance,
		m_accelerationStructures.top.topInstanceMemory);

	void* pInstances;
	vkMapMemory(m_device->handle(), m_accelerationStructures.top.topInstanceMemory, 0, sizeof(VkGeometryInstance) * 1, 0, &pInstances);
	memcpy(pInstances, &geometryInstance, sizeof(VkGeometryInstance));
	vkUnmapMemory(m_device->handle(), m_accelerationStructures.top.topInstanceMemory);

	VkBindAccelerationStructureMemoryInfoNV bindInfo = {};
	bindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	bindInfo.pNext = nullptr;
	bindInfo.accelerationStructure = m_accelerationStructures.top.handle;
	bindInfo.memory = m_accelerationStructures.top.resultMemory;
	bindInfo.memoryOffset = 0;
	bindInfo.deviceIndexCount = 0;
	bindInfo.pDeviceIndices = nullptr;

	if (m_device->getExtensions().vkBindAccelerationStructureMemoryNV(m_device->handle(), 1, &bindInfo) != VK_SUCCESS) {
		std::cerr << "failed to create top acceleration structure!" << std::endl;
		return false;
	}

	VkAccelerationStructureInfoNV buildInfo = {};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.pNext = nullptr;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV; //VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.instanceCount = 1;
	buildInfo.geometryCount = 0;
	buildInfo.pGeometries = nullptr;

	m_device->getExtensions().vkCmdBuildAccelerationStructureNV(
		cmd, &buildInfo,
		m_accelerationStructures.top.topInstance,
		0,
		false,
		m_accelerationStructures.top.handle,
		nullptr,
		m_accelerationStructures.top.build,
		0);

	return true;
}

AccelerationStructures RaytracingAccelerationStructureBuilder::build(Model& model) {
	Queue* pQueue = m_device->getQueue(QueueType::eGraphics);
	VkCommandBuffer cmd = pQueue->beginSingleTimeCommands();

	createBottomLevelAccelerationStructure(cmd, model);
	
	// Wait for the builder to complete by setting a barrier on the resulting buffer. This is
		// particularly important as the construction of the top-level hierarchy may be called right
		// afterwards, before executing the command list.
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	createTopLevelAccelerationStructure(cmd);

	pQueue->endSingleTimeCommands(cmd);

	return m_accelerationStructures;
}

}
