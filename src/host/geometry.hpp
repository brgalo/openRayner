#pragma once
#include "device.hpp"
#include "glm/fwd.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace oray {
class Geometry {
public:
  struct Vertex {
    glm::vec2 position;
    glm::vec3 color;
    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
  };

  Geometry(Device& device, const std::vector<Vertex> &vertices);
  ~Geometry();

  Geometry(const Geometry &) = delete;
  Geometry &operator=(const Geometry &) = delete;

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

private:
  void createVertexBuffers(const std::vector<Vertex> &vertices);
  Device &device;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  uint32_t vertexCount;
};
}