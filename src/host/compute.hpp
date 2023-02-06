#include "device.hpp"
#include "geometry.hpp"
#include "orayobject.hpp"

#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>
namespace oray {
class Compute {
public:
  Compute(Device &device, std::vector<OrayObject> const &orayObjects);
  struct PushConsts {
    uint64_t inBuffer;
    uint64_t outBuffer;
    uint64_t nTriangleToMeshIdxBuffer;
    uint64_t nMeshes;
    uint64_t nTriangles;
    uint64_t launchSize;
  } pushConsts;
  void compute(VkCommandBuffer commandBuffer);
private:
  Device &device;
  void createPipelineLayout();
  void createPipeline();
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  std::unique_ptr<Buffer> energyBuffer;
  std::unique_ptr<Buffer> meshEnergyBuffer;
};
} // namespace oray