#pragma once

#include "buffer.hpp"
#include "camera.hpp"
#include "frameinfo.hpp"
#include "geometry.hpp"
#include "orayobject.hpp"
#include "pipeline.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {
class RenderSystem {
public:
  RenderSystem(Device &device, VkRenderPass triRenderPass,
               VkRenderPass lineRenderPass,
               VkDescriptorSetLayout globalSetLayout);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  void renderOrayObjects(FrameInfo &frameInfo,
                         std::vector<OrayObject> &orayObjects);
  void renderLines(FrameInfo &frameInfo, Buffer &lines);

private:
  void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
  void createPipeline(VkRenderPass renderPass,
                      std::unique_ptr<Pipeline> &pipeline,
                      VkPipelineLayout pipelineLayout,
                      PipelineConfigInfo &configInfo,
                      const std::string vertShaderFilepath,
                      const std::string fragShaderFilepath);

  Device &device;

  std::unique_ptr<Pipeline> trianglePipeline;
  std::unique_ptr<Pipeline> linePipeline;
  VkPipelineLayout trianglePipelineLayout;
  VkPipelineLayout linePipelineLayout;
};

} // namespace oray