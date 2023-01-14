#include "functions.hpp"
#include <vulkan/vulkan_core.h>

namespace oray {
VulkanFunctions::VulkanFunctions(VkDevice device) {
  vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
      vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
  vkGetAccelerationStructureBuildSizesKHR =
      reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
          vkGetDeviceProcAddr(device,
                              "vkGetAccelerationStructureBuildSizesKHR"));
  vkCreateAccelerationStructureKHR =
      reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
          vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
  vkCmdBuildAccelerationStructuresKHR =
      reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
          vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
  vkGetAccelerationStructureDeviceAddressKHR =
      reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
          vkGetDeviceProcAddr(device,
                              "vkGetAccelerationStructureDeviceAddressKHR"));
  vkDestroyAccelerationStructureKHR =
      reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
          vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
  vkCreateRayTracingPipelinesKHR =
      reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
          vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
  vkGetRayTracingShaderGroupHandlesKHR =
      reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
          vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
}

VulkanFunctions::~VulkanFunctions() {
  
}

} // namespace oray