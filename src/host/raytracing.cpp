#include "raytracing.hpp"
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "pipeline.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace oray {

Raytracer::Raytracer(Device &device, std::vector<OrayObject> &orayObjects)
    : device(device) {
  buildBLAS(orayObjects);
  buildTLAS();
  createDescriptorSetLayout();
  createDescriptorPool();
  createShaderModules();
  createRtPipelineLayout();
  createRtPipeline();
}

Raytracer::~Raytracer() {
  f.vkDestroyAccelerationStructureKHR(device.device(), blas, nullptr);
}

void Raytracer::buildBLAS(std::vector<OrayObject> &orayObjects) {
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
  instanceBuffer =
      std::make_unique<Buffer>(device, sizeof(instance), 1,
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
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

void Raytracer::createDescriptorSetLayout() {
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  bindings[1] = bindings[0];
  bindings[1].binding = 2;

  bindings[2] = bindings[0];
  bindings[2].binding = 3;

  bindings[3] = bindings[0];
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  bindings[3].binding = 1;

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create rt descriptor set layout!");
  }
}

void Raytracer::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes;
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[0].descriptorCount = 3;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  poolSizes[1].descriptorCount = 1;

  VkDescriptorPoolCreateInfo descriptorPoolInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  descriptorPoolInfo.maxSets = 1;
  descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolInfo.pPoolSizes = poolSizes.data();
  if (vkCreateDescriptorPool(device.device(), &descriptorPoolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create rt desctiptor pool!");
  };
}

void Raytracer::createShaderModules() {
  rayGenShader = createShaderModule("spv/raygen.rgen.spv");
  chShader = createShaderModule("spv/closesthit.rchit.spv");
  missShader = createShaderModule("spv/raygen.rgen.spv");
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
  createInfo.pSetLayouts = &descriptorSetLayout;

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

  rtsgci[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  rtsgci[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  rtsgci[1].closestHitShader = 1;

  rtsgci[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  rtsgci[2].generalShader = 2;

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
}

} // namespace oray