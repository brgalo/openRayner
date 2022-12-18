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
  struct Vertex {
    glm::vec3 position{};
    glm::vec3 color{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();

    bool operator==(const Vertex &other) const{
      return position == other.position && color == other.color &&
             normal == other.normal && uv == other.uv;
    }
  };


  struct Builder {
    std::vector<Vertex> vertices{};
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
  void createVertexBuffers(const std::vector<Vertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);
  
  Device &device;
std::unique_ptr<Buffer> vertexBuffer;
  uint32_t vertexCount;

  bool hasIndexBuffer = false;
std::unique_ptr<Buffer> indexBuffer;
  uint32_t indexCount;
};
} // namespace oray