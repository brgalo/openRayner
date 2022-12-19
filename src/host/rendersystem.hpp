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
  RenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  void renderOrayObjects(FrameInfo &frameInfo,
                         std::vector<OrayObject> &orayObjects);

private:
  void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
  void createPipeline(VkRenderPass renderPass,
                      std::unique_ptr<Pipeline> &pipeline,
                      const std::string vertShaderFilepath,
                      const std::string fragShaderFilepath);

  Device &device;

  std::unique_ptr<Pipeline> trianglePipeline;
  std::unique_ptr<Pipeline> linePipeline;
  VkPipelineLayout pipelineLayout;
};

} // namespace oray