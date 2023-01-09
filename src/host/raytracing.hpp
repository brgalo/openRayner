#pragma once
#include "buffer.hpp"
#include "device.hpp"
#include "orayobject.hpp"
#include "functions.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace oray {
class Raytracer {
public:
  Raytracer(Device &device,
            std::vector<OrayObject> &orayObjects);
  ~Raytracer();
  

private:
  Device &device;
  VulkanFunctions f = VulkanFunctions(device.device());
  //std::shared_ptr<const std::vector<OrayObject>> orayObjects;
  std::unique_ptr<Buffer> blasBuffer;
  VkAccelerationStructureKHR blas;
  std::unique_ptr<Buffer> instanceBuffer;
  VkAccelerationStructureKHR tlas;
  void buildAccelerationStructure(std::vector<OrayObject> &orayObjects);

  VkAccelerationStructureInstanceKHR instance{};
};

} // namespace oray
