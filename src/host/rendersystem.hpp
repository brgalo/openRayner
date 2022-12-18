#pragma once

#include "camera.hpp"
#include "frameinfo.hpp"
#include "geometry.hpp"
#include "orayobject.hpp"
#include "pipeline.hpp"

#include <memory>
#include <vector>

namespace oray {
class RenderSystem {
public:
  RenderSystem(Device &device, VkRenderPass renderPass);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  void renderOrayObjects(FrameInfo &frameInfo,
                         std::vector<OrayObject> &orayObjects);

private:
  void createPipelineLayout();
  void createPipeline(VkRenderPass renderPass);

  Device &device;

  std::unique_ptr<Pipeline> graphicsPipeline;
  VkPipelineLayout pipelineLayout;
};

} // namespace oray