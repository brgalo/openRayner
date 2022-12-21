#include "geometry.hpp"
#include "buffer.hpp"
#include "device.hpp"
#include "utils.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
template <> struct hash<oray::Geometry::TriangleVertex> {
  size_t operator()(oray::Geometry::TriangleVertex const &vertex) const {
    size_t seed = 0;
    oray::hashCombine(seed, vertex.position, vertex.color, vertex.normal,
                      vertex.uv);
    return seed;
  } 
};
}

namespace oray {
using std::vector;

Geometry::Geometry(Device &device, const Geometry::Builder &builder)
    : device(device) {
  createVertexBuffers(builder.vertices);
  createIndexBuffers(builder.indices);
}

Geometry::~Geometry() {}

std::unique_ptr<Geometry>
Geometry::createModelFromFile(Device &device, const std::string &filePath) {
  Builder builder{};
  builder.loadModel(filePath);
  return std::make_unique<Geometry>(device, builder);
}

void Geometry::createVertexBuffers(const std::vector<TriangleVertex> &vertices) {
  vertexCount = static_cast<uint32_t>(vertices.size());
  assert(vertexCount >= 3 && "Vertex count must be at least 3");
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
  uint32_t vertexSize = sizeof(vertices[0]);

  Buffer stagingBuffer {
    device, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)vertices.data());

  vertexBuffer = std::make_unique<Buffer>(device, vertexSize, vertexCount,
                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Geometry::createIndexBuffers(const std::vector<uint32_t> &indices) {
  indexCount = static_cast<uint32_t>(indices.size());
  hasIndexBuffer = indexCount > 0;

  if (!hasIndexBuffer) {
    return;
  }

  VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
  uint32_t indexSize = sizeof(indices[0]);

  Buffer stagingBuffer {
    device, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)indices.data());

  indexBuffer = std::make_unique<Buffer>(device, indexSize, indexCount,
                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

void Geometry::bind(VkCommandBuffer commandBuffer) {
  std::vector<VkBuffer> buffers = {vertexBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data(), offsets);

  if (hasIndexBuffer) {
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
  }
}

void Geometry::draw(VkCommandBuffer commandBuffer) {
  if (hasIndexBuffer) {
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
  } else {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }
}

vector<VkVertexInputBindingDescription>
Geometry::getBindingDescriptionsTriangle() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(TriangleVertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}

vector<VkVertexInputBindingDescription>
Geometry::getBindingDescriptionsLine() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(LineVertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}


std::vector<VkVertexInputAttributeDescription>
Geometry::getAttributeDescriptionsTriangle() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TriangleVertex, position)});
  attributeDescriptions.push_back(
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TriangleVertex, color)});
  attributeDescriptions.push_back(
      {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TriangleVertex, normal)});
  attributeDescriptions.push_back(
      {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(TriangleVertex, uv)});
  return attributeDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
Geometry::getAttributeDescriptionsLine() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LineVertex, position)});
  attributeDescriptions.push_back(
      {1, 0, VK_FORMAT_R32_SFLOAT, offsetof(LineVertex, distFromStart)});
  return attributeDescriptions;
}



void Geometry::Builder::loadModel(const std::string &filePath) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        filePath.c_str())) {
    throw std::runtime_error(warn + err);
  }

  vertices.clear();
  indices.clear();

  std::unordered_map<TriangleVertex, uint32_t> uniqueVertices{};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      TriangleVertex vertex{};

      if (index.vertex_index >= 0) {
        vertex.position = {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2],
        };

        vertex.color = {
          attrib.colors[3 * index.vertex_index + 0],
          attrib.colors[3 * index.vertex_index + 1],
          attrib.colors[3 * index.vertex_index + 2],
        };
      }

      if (index.normal_index >= 0) {
        vertex.normal = {
            attrib.normals[3 * index.normal_index + 0],
            attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
        };
      }

      if (index.texcoord_index >= 0) {
        vertex.uv = {
            attrib.texcoords[2 * index.texcoord_index + 0],
            attrib.texcoords[2 * index.texcoord_index + 1],
        };
      }

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }
}
} // namespace oray