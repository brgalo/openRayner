#pragma once
#include "buffer.hpp"
#include "device.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace oray {
class Geometry {
public:
  static std::vector<VkVertexInputBindingDescription>
  getBindingDescriptionsTriangle();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptionsTriangle();
  static std::vector<VkVertexInputBindingDescription>
  getBindingDescriptionsLine();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptionsLine();

  struct TriangleVertex {
    glm::vec3 position{};
    glm::vec3 color{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    bool operator==(const TriangleVertex &other) const {
      return position == other.position && color == other.color &&
             normal == other.normal && uv == other.uv;
    }
  };

  struct MeshIdx {
    // .x = nTrianglesInCurrentMesh
    // .y = cumTrianglesWithCurrentMesh
    glm::uvec4 data{0};
  };

  struct Builder {
    std::vector<TriangleVertex> vertices{};
    std::vector<uint32_t> indices{};
    std::vector<MeshIdx> triangleToMeshIdx{};

    void loadModel(const std::string &filePath);
  };

  Geometry(Device &device, const Geometry::Builder &builder);
  ~Geometry();

  Geometry(const Geometry &) = delete;
  Geometry &operator=(const Geometry &) = delete;

  static std::unique_ptr<Geometry>
  createModelFromFile(Device &device, const std::string &filePath);

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

  uint32_t getIndexCount() { return indexCount; };

  VkDeviceAddress getIndexBufferAddress();
  VkDeviceAddress getVertexBufferAddress();
  VkDeviceAddress getTriangleToMeshIdxBufferAddress();
  const std::vector<MeshIdx> &triToMeshIdx() {
    return static_cast < const std::vector<MeshIdx>&>(triangleToMeshIdx);
  };
private:
  void createVertexBuffers(const std::vector<TriangleVertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);
  void createMeshIdxBuffer(const std::vector<MeshIdx> &tri2MeshIdx);

  Device &device;
  std::unique_ptr<Buffer> vertexBuffer;
  uint32_t vertexCount;

  bool hasIndexBuffer = false;
  std::unique_ptr<Buffer> indexBuffer;
  uint32_t indexCount;

  std::unique_ptr<Buffer> meshIdxBuffer;
  std::vector<MeshIdx> triangleToMeshIdx{};
};
} // namespace oray