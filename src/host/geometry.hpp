#pragma once
#include "device.hpp"
#include "buffer.hpp"

#include <vector>
#include <memory>

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

    bool operator==(const TriangleVertex &other) const{
      return position == other.position && color == other.color &&
             normal == other.normal && uv == other.uv;
    }
  };

  struct LineVertex {
    glm::vec3 position{};
    float distFromStart{};
  };


  struct Builder {
    std::vector<TriangleVertex> vertices{};
    std::vector<uint32_t> indices{};

    void loadModel(const std::string &filePath);
  };

  Geometry(Device &device, const Geometry::Builder & builder);
  ~Geometry();


  Geometry(const Geometry &) = delete;
  Geometry &operator=(const Geometry &) = delete;

  static std::unique_ptr<Geometry>
  createModelFromFile(Device &device, const std::string &filePath);
  

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

private:
  void createVertexBuffers(const std::vector<TriangleVertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);
  
  Device &device;
std::unique_ptr<Buffer> vertexBuffer;
  uint32_t vertexCount;

  bool hasIndexBuffer = false;
std::unique_ptr<Buffer> indexBuffer;
  uint32_t indexCount;
};
} // namespace oray