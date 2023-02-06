#include <cstdint>
#include <vulkan/vulkan_core.h>
namespace oray {

struct RtPushConstants {
  uint64_t indexBuffer;
  uint64_t vertexBuffer;
  uint64_t oriBuffer;
  uint64_t dirBuffer;
  uint64_t hitBuffer;
  uint64_t triangleIndex;
  uint64_t nTriangles;
  uint64_t tri2MeshIdxBuffer;
//  bool recordDir;
//  bool recordHit;
};

}