#include "raytracing.hpp"
#include "buffer.hpp"
#include "device.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace oray {

Raytracer::Raytracer(Device &device,
                     std::vector<OrayObject> &orayObjects)
    : device(device) {
  buildAccelerationStructure(orayObjects);
}

Raytracer::~Raytracer() {
  f.vkDestroyAccelerationStructureKHR(device.device(), blas, nullptr);
}

void Raytracer::buildAccelerationStructure(std::vector<OrayObject> &orayObjects) {
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
      0, 0);

  VkAccelerationStructureCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  createInfo.type = buildInfo.type;
  createInfo.size = sizeInfo.accelerationStructureSize;
  createInfo.buffer = blasBuffer->getBuffer();
  createInfo.offset = 0;
  if (f.vkCreateAccelerationStructureKHR(device.device(), &createInfo, nullptr,
                                       &blas) != VK_SUCCESS) {
    throw std::runtime_error("failed to create acceleration structure!");
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

  VkAccelerationStructureDeviceAddressInfoKHR adressInfo{};
  adressInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  adressInfo.accelerationStructure = blas;
  VkDeviceAddress blasAdress =
      f.vkGetAccelerationStructureDeviceAddressKHR(device.device(), &adressInfo);

  instance.transform.matrix[0][0] = 1.f;
  instance.transform.matrix[1][1] = 1.f;
  instance.transform.matrix[2][2] = 1.f;
  instance.instanceCustomIndex = 0;
  instance.mask = 0xFF;
  instance.instanceShaderBindingTableRecordOffset = 0;
  instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  instance.accelerationStructureReference = blasAdress;

  instanceBuffer =
      std::make_unique<Buffer>(device, sizeof(instance), 1,
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 0);
}

} // namespace oray