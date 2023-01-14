#include "raytracing.hpp"
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "bindings.h"

namespace oray {

Raytracer::Raytracer(Device &device, std::vector<OrayObject> const &orayObjects)
    : device(device) {
  buildBLAS(orayObjects);
  buildTLAS();
  rtDescriptorSetLayout = createDescriptorSetLayout();
  rtDescriptorPool = createDescriptorPool();
  createShaderModules();
  createRtPipelineLayout();
  createRtPipeline();
  initPushConstants(orayObjects);
}

Raytracer::~Raytracer() {
  f.vkDestroyAccelerationStructureKHR(device.device(), blas, nullptr);
}

uint32_t Raytracer::alignUp(uint32_t val, uint32_t align) {
  uint32_t remainder = val % align;
  if (remainder == 0)
    return val;
  return val + align - remainder;
}

void Raytracer::buildBLAS(std::vector<OrayObject> const &orayObjects) {
  VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
  triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress =
      orayObjects.at(0).geom->getVertexBufferAddress();
  triangles.vertexStride =
      static_cast<uint32_t>(sizeof(Geometry::TriangleVertex));
  triangles.indexType = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress =
      orayObjects.at(0).geom->getIndexBufferAddress();
  triangles.maxVertex = orayObjects.at(0).geom->getIndexCount();
  triangles.transformData = {0};

  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  geometry.geometry.triangles = triangles;
  geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

  VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
  rangeInfo.firstVertex = 0;
  rangeInfo.primitiveCount = orayObjects.at(0).geom->getIndexCount() / 3;
  rangeInfo.primitiveOffset = 0;
  rangeInfo.transformOffset = 0;

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
  buildInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildInfo.geometryCount = 1;
  buildInfo.pGeometries = &geometry;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
  sizeInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  f.vkGetAccelerationStructureBuildSizesKHR(
      device.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &buildInfo, &rangeInfo.primitiveCount, &sizeInfo);
  blasBuffer = std::make_unique<Buffer>(
      device, sizeInfo.accelerationStructureSize, 1,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0);

  VkAccelerationStructureCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  createInfo.type = buildInfo.type;
  createInfo.size = sizeInfo.accelerationStructureSize;
  createInfo.buffer = blasBuffer->getBuffer();
  createInfo.offset = 0;
  if (f.vkCreateAccelerationStructureKHR(device.device(), &createInfo, nullptr,
                                         &blas) != VK_SUCCESS) {
    throw std::runtime_error("failed to create blas!");
  }
  buildInfo.dstAccelerationStructure = blas;

  // scratch buffer
  Buffer scratchBuffer = Buffer{device, sizeInfo.buildScratchSize, 1,
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                0};
  buildInfo.scratchData.deviceAddress = scratchBuffer.getAddress();

  VkAccelerationStructureBuildRangeInfoKHR *pRangeInfo = &rangeInfo;

  VkCommandBuffer cmdBuffer = device.beginSingleTimeCommands();
  f.vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);
  device.endSingleTimeCommands(cmdBuffer);
  vkDeviceWaitIdle(device.device());
}

void Raytracer::buildTLAS() {
  VkAccelerationStructureDeviceAddressInfoKHR adressInfo{};
  adressInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  adressInfo.accelerationStructure = blas;
  VkDeviceAddress blasAdress = f.vkGetAccelerationStructureDeviceAddressKHR(
      device.device(), &adressInfo);

  instance.transform.matrix[0][0] = 1.f;
  instance.transform.matrix[1][1] = 1.f;
  instance.transform.matrix[2][2] = 1.f;
  instance.instanceCustomIndex = 0;
  instance.mask = 0xFF;
  instance.instanceShaderBindingTableRecordOffset = 0;
  instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  instance.accelerationStructureReference = blasAdress;

  Buffer stagingBuffer =
      Buffer(device, sizeof(instance), 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();
  stagingBuffer.writeToBuffer(&instance);
  instanceBuffer = std::make_unique<Buffer>(
      device, sizeof(instance), 1,
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  device.copyBuffer(stagingBuffer.getBuffer(), instanceBuffer->getBuffer(),
                    sizeof(instance));
  vkDeviceWaitIdle(device.device());

  VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
  rangeInfo.primitiveOffset = 0;
  rangeInfo.primitiveCount = 1;
  rangeInfo.firstVertex = 0;
  rangeInfo.transformOffset = 0;

  VkAccelerationStructureGeometryInstancesDataKHR instancesVK{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
  instancesVK.data.deviceAddress = instanceBuffer->getAddress();
  instancesVK.arrayOfPointers = VK_FALSE;

  VkAccelerationStructureGeometryKHR geometry{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry.instances = instancesVK;

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  buildInfo.geometryCount = 1;
  buildInfo.pGeometries = &geometry;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

  f.vkGetAccelerationStructureBuildSizesKHR(
      device.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &buildInfo, &rangeInfo.primitiveCount, &sizeInfo);

  tlasBuffer = std::make_unique<Buffer>(
      device, sizeInfo.accelerationStructureSize, 1,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0);

  VkAccelerationStructureCreateInfoKHR createInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
  createInfo.type = buildInfo.type;
  createInfo.size = sizeInfo.accelerationStructureSize;
  createInfo.buffer = tlasBuffer->getBuffer();
  createInfo.offset = 0;
  if (f.vkCreateAccelerationStructureKHR(device.device(), &createInfo, nullptr,
                                         &tlas) != VK_SUCCESS) {
    throw std::runtime_error("failed to create tlas!");
  }

  buildInfo.dstAccelerationStructure = tlas;

  Buffer scratchBuffer = Buffer(device, sizeInfo.buildScratchSize, 1,
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                0);
  buildInfo.scratchData.deviceAddress = scratchBuffer.getAddress();

  VkAccelerationStructureBuildRangeInfoKHR *pRangeInfo = &rangeInfo;

  VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
  f.vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo,
                                        &pRangeInfo);
  device.endSingleTimeCommands(commandBuffer);
  vkDeviceWaitIdle(device.device());
}

std::unique_ptr<DescriptorSetLayout> Raytracer::createDescriptorSetLayout() {
  return DescriptorSetLayout::Builder(device)
      .addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                  VK_SHADER_STAGE_RAYGEN_BIT_KHR)
      .build();
}

std::unique_ptr<DescriptorPool> Raytracer::createDescriptorPool() {
  return DescriptorPool::Builder(device)
      .addPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1)
      .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
      .setMaxSets(1)
      .build();
}

void Raytracer::createShaderModules() {
  rayGenShader = createShaderModule("spv/raygen.rgen.spv");
  chShader = createShaderModule("spv/closesthit.rchit.spv");
  missShader = createShaderModule("spv/miss.rmiss.spv");
}

VkShaderModule Raytracer::createShaderModule(const std::string &filepath) {
  VkShaderModule ret;
  auto code = Pipeline::readFile(filepath);
  VkShaderModuleCreateInfo createInfo{
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = static_cast<uint32_t>(code.size());
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr, &ret) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create RT shader module!");
  }
  return ret;
}

void Raytracer::createRtPipelineLayout() {
  VkPipelineLayoutCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  createInfo.setLayoutCount = 1;
  createInfo.pSetLayouts =
      rtDescriptorSetLayout->getDescriptorSetLayoutPointer();

  VkPushConstantRange constantsRanges{};
  constantsRanges.size = sizeof(RtPushConstants);
  constantsRanges.offset = 0;
  constantsRanges.stageFlags =
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  createInfo.pushConstantRangeCount = 1;
  createInfo.pPushConstantRanges = &constantsRanges;

  if (vkCreatePipelineLayout(device.device(), &createInfo, nullptr,
                             &rtPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create rt pipeline layout!");
  }
}

void Raytracer::createRtPipeline() {
  // TODO: rename shader entry points to correspond to programm!
  std::array<VkPipelineShaderStageCreateInfo, 3> pssci{};
  pssci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci[0].module = rayGenShader;
  pssci[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  pssci[0].pName = "main";

  pssci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci[1].module = chShader;
  pssci[1].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  pssci[1].pName = "main";

  pssci[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci[2].module = missShader;
  pssci[2].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
  pssci[2].pName = "main";

  std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> rtsgci{};
  rtsgci[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  rtsgci[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  rtsgci[0].generalShader = 0;
  rtsgci[0].closestHitShader = VK_SHADER_UNUSED_KHR;
  rtsgci[0].anyHitShader = VK_SHADER_UNUSED_KHR;
  rtsgci[0].intersectionShader = VK_SHADER_UNUSED_KHR;

  rtsgci[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  rtsgci[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  rtsgci[1].generalShader = VK_SHADER_UNUSED_KHR;
  rtsgci[1].closestHitShader = 1;
  rtsgci[1].anyHitShader = VK_SHADER_UNUSED_KHR;
  rtsgci[1].intersectionShader = VK_SHADER_UNUSED_KHR;

  rtsgci[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  rtsgci[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  rtsgci[2].generalShader = 2;
  rtsgci[2].closestHitShader = VK_SHADER_UNUSED_KHR;
  rtsgci[2].anyHitShader = VK_SHADER_UNUSED_KHR;
  rtsgci[2].intersectionShader = VK_SHADER_UNUSED_KHR;

  VkRayTracingPipelineCreateInfoKHR rtpci{
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
  rtpci.stageCount = static_cast<uint32_t>(pssci.size());
  rtpci.pStages = pssci.data();
  rtpci.groupCount = static_cast<uint32_t>(rtsgci.size());
  rtpci.pGroups = rtsgci.data();
  rtpci.maxPipelineRayRecursionDepth = 0;
  rtpci.layout = rtPipelineLayout;

  if (f.vkCreateRayTracingPipelinesKHR(device.device(), VK_NULL_HANDLE,
                                       VK_NULL_HANDLE, 1, &rtpci, nullptr,
                                       &rtPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to createpipeline!");
  }

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProps{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  VkPhysicalDeviceProperties2 prop2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  prop2.pNext = &rtPipelineProps;
  vkGetPhysicalDeviceProperties2(device.getPhysicalDevice(), &prop2);

  // https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#raytracing

  uint32_t missCount = 1;
  uint32_t hitCount = 1;
  uint32_t handleCount = static_cast<uint32_t>(rtsgci.size());
  uint32_t handleSize = rtPipelineProps.shaderGroupHandleSize;
  uint32_t handleSizeAligned =
      alignUp(handleSize, rtPipelineProps.shaderGroupHandleAlignment);

  rgenRegion.stride =
      alignUp(handleSizeAligned, rtPipelineProps.shaderGroupBaseAlignment);
  rgenRegion.size = rgenRegion.stride;
  missRegion.stride = handleSizeAligned;
  missRegion.size = alignUp(missCount * handleSizeAligned,
                            rtPipelineProps.shaderGroupBaseAlignment);
  hitRegion.stride = handleSizeAligned;
  hitRegion.size = alignUp(hitCount * handleSizeAligned,
                           rtPipelineProps.shaderGroupBaseAlignment);

  uint32_t dataSize = handleCount * handleSize;
  handles = std::vector<uint8_t>(dataSize);
  if (f.vkGetRayTracingShaderGroupHandlesKHR(device.device(), rtPipeline, 0,
                                           handleCount, dataSize,
                                           handles.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to retrieve shader group handles!");
  }
  VkDeviceSize sbtSize = rgenRegion.size + missRegion.size + hitRegion.size;
  sbtBuffer =
      std::make_unique<Buffer>(device, sbtSize, 1,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  
  VkDeviceAddress sbtAdress = sbtBuffer->getAddress();
  rgenRegion.deviceAddress = sbtAdress;
  hitRegion.deviceAddress = sbtAdress + rgenRegion.size;
  missRegion.deviceAddress = sbtAdress + rgenRegion.size + hitRegion.size;

  std::vector<uint8_t> sbtDataHost(sbtSize);
  uint32_t handleIdx = 0;
  uint8_t *pData = sbtDataHost.data();

  auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

  //raygen
  memcpy(pData, getHandle(handleIdx++), handleSize);

  // chit
  pData = sbtDataHost.data() + rgenRegion.size;
  for (uint32_t c = 0; c < hitCount; ++c) {
    memcpy(pData, getHandle(handleIdx++), handleSize);
    pData += hitRegion.stride;
  }

  // miss
  pData = sbtDataHost.data() + rgenRegion.size + hitRegion.size;
  for (uint32_t c = 0; c < missCount; ++c) {
    memcpy(pData, getHandle(handleIdx++), handleSize);
  }
  sbtBuffer->map();
  sbtBuffer->writeToBuffer(sbtDataHost.data());
  sbtBuffer->unmap();
}

void Raytracer::initPushConstants(std::vector<OrayObject> const &orayObjects) {
        DescriptorWriter(*rtDescriptorSetLayout, *rtDescriptorPool)
      .writeandBuildTLAS(0, &tlas,
                         rtDescriptorSet);
  

  pushConstants.indexBuffer = orayObjects[0].geom->getIndexBufferAddress();
  pushConstants.vertexBuffer = orayObjects[0].geom->getVertexBufferAddress();
}

void Raytracer::traceTriangle(VkCommandBuffer cmdBuf, uint64_t nRays,
                              uint64_t triangleIdx, bool recOri, bool recDir,
                              bool recHit) {
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          rtPipelineLayout, 0, 1, &rtDescriptorSet, 0,
                          VK_NULL_HANDLE);
  vkCmdPushConstants(cmdBuf, rtPipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                     0, static_cast<uint32_t>(sizeof(pushConstants)),
                     &pushConstants);
  f.vkCmdTraceRaysKHR(cmdBuf, &rgenRegion, &missRegion, &hitRegion, &callRegion,
                    10, 1, 1);
}

} // namespace oray