#pragma once
#include "buffer.hpp"
#include "device.hpp"
#include "orayobject.hpp"
#include "functions.hpp"
#include <array>
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
  std::unique_ptr<Buffer> tlasBuffer;
  VkAccelerationStructureKHR tlas;

  std::unique_ptr<Buffer> instanceBuffer;

  std::array<VkDescriptorSetLayoutBinding, 4> bindings;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;

  VkShaderModule rayGenShader;
  VkShaderModule chShader;
  VkShaderModule missShader;

  VkPipelineLayout rtPipelineLayout;
  VkPipeline rtPipeline;

  void buildBLAS(std::vector<OrayObject> &orayObjects);
  void buildTLAS();
  void createDescriptorSetLayout();
  void createDescriptorPool();
  void createShaderModules();
  void createRtPipelineLayout();
  void createRtPipeline();

  VkShaderModule createShaderModule(const std::string &filepath);

  VkAccelerationStructureInstanceKHR instance{};
};

} // namespace oray
