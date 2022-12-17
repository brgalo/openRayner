#pragma once

#include "pipeline.hpp"
#include "geometry.hpp"
#include "orayobject.hpp"

#include <memory>
#include <vector>

namespace oray {
class RenderSystem {
public:
  RenderSystem(Device& device, VkRenderPass renderPass);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  void renderOrayObjects(VkCommandBuffer commandBuffer, std::vector<OrayObject> &orayObejcts);
private:
  void createPipelineLayout();
  void createPipeline(VkRenderPass renderPass);

  Device &device;
  
  std::unique_ptr<Pipeline> graphicsPipeline;
  VkPipelineLayout pipelineLayout;
};

} // namespace oray