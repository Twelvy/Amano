#include "Extensions.h"

#define GET_PROC_ADDRESS(proc_name) proc_name = (PFN_##proc_name)vkGetInstanceProcAddr(instance, #proc_name)

namespace Amano {

bool Extensions::queryRaytracingFunctions(VkInstance instance) {
	GET_PROC_ADDRESS(vkCreateRayTracingPipelinesKHR);
	GET_PROC_ADDRESS(vkCmdTraceRaysKHR);
	vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR");
	vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR");
	GET_PROC_ADDRESS(vkGetAccelerationStructureBuildSizesKHR);
	GET_PROC_ADDRESS(vkGetAccelerationStructureDeviceAddressKHR);
	GET_PROC_ADDRESS(vkCmdBuildAccelerationStructuresKHR);
	GET_PROC_ADDRESS(vkGetRayTracingShaderGroupHandlesKHR);
	vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");

	return true;
}

}
