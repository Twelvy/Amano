#include "Extensions.h"

namespace Amano {

bool Extensions::queryRaytracingFunctions(VkInstance instance) {
	vkCreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV)vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesNV");
	vkBindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV)vkGetInstanceProcAddr(instance, "vkBindAccelerationStructureMemoryNV");
	vkCmdTraceRaysNV = (PFN_vkCmdTraceRaysNV)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysNV");
	vkCompileDeferredNV = (PFN_vkCompileDeferredNV)vkGetInstanceProcAddr(instance, "vkCompileDeferredNV");
	vkCreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV)vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureNV");
	vkDestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV)vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureNV");
	vkGetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureMemoryRequirementsNV");
	vkGetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureHandleNV");
	vkCmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV)vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructureNV");
	vkGetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesNV");
	vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");

	return true;
}

}
