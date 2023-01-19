#pragma once
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "functions.hpp"
#include "glm/glm.hpp"
#include "orayobject.hpp"

#include "commonStructs.h"
#include "state.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace oray {
class State;
class Raytracer {
public:
  Raytracer(Device &device, std::vector<OrayObject> const &orayObjects, std::shared_ptr<State> state);
  ~Raytracer();
  void traceTriangle(VkCommandBuffer cmdBuf);
  std::vector<glm::vec4> readOutputBuffer() {
    return returnBuffer(*outputBuffer);
  };
  std::vector<glm::vec4> readDirBuffer() {
    return returnBuffer(*dirBuffer);
  };
  std::vector<glm::vec4> readOriBuffer() { return returnBuffer(*oriBuffer); };
  Buffer &getOriBuffer() { return *oriBuffer; };

  RtPushConstants* pushConsts() {return &pushConstants;};
  std::shared_ptr<State> state;
  
private:
  const uint32_t nTrinagles;
  Device &device;
  VulkanFunctions f = VulkanFunctions(device.device());
  // std::shared_ptr<const std::vector<OrayObject>> orayObjects;
  std::unique_ptr<Buffer> blasBuffer;
  VkAccelerationStructureKHR blas;
  std::unique_ptr<Buffer> tlasBuffer;
  VkAccelerationStructureKHR tlas;
  std::unique_ptr<Buffer> sbtBuffer;

  std::unique_ptr<Buffer> outputBuffer;
  std::unique_ptr<Buffer> oriBuffer;
  std::unique_ptr<Buffer> dirBuffer;

  std::unique_ptr<Buffer> instanceBuffer;

  std::unique_ptr<DescriptorSetLayout> rtDescriptorSetLayout;
  std::unique_ptr<DescriptorPool> rtDescriptorPool;

  RtPushConstants pushConstants;

  VkShaderModule rayGenShader;
  VkShaderModule chShader;
  VkShaderModule missShader;

  VkPipelineLayout rtPipelineLayout;
  VkPipeline rtPipeline;

  VkDescriptorSet rtDescriptorSet;

  VkStridedDeviceAddressRegionKHR rgenRegion{};
  VkStridedDeviceAddressRegionKHR hitRegion{};
  VkStridedDeviceAddressRegionKHR missRegion{};
  VkStridedDeviceAddressRegionKHR callRegion{};

  std::vector<uint8_t> handles{};

  void buildBLAS(std::vector<OrayObject> const &orayObjects);
  void buildTLAS();
  std::unique_ptr<DescriptorSetLayout> createDescriptorSetLayout();
  std::unique_ptr<DescriptorPool> createDescriptorPool();
  void createShaderModules();
  void createRtPipelineLayout();
  void createRtPipeline();
  void createShaderBindingTable();
  void initPushConstants(std::vector<OrayObject> const &orayObjects);
  void resizeBuffers();
  VkShaderModule createShaderModule(const std::string &filepath);

  VkAccelerationStructureInstanceKHR instance{};
  uint32_t alignUp(uint32_t val, uint32_t align);

  std::vector<glm::vec4> returnBuffer(Buffer &buffer);
};

} // namespace oray
