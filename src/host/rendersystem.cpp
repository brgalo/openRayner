#include "rendersystem.hpp"
#include "glm/fwd.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "GLFW/glfw3.h"
#include "geometry.hpp"
#include "pipeline.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {

struct SimplePushConstantData {
  glm::mat4 transform{1.f};
  glm::mat4 normalMatrix{1.f};
};

RenderSystem::RenderSystem(Device &device, VkRenderPass renderPass)
    : device{device} {
  createPipelineLayout();
  createPipeline(renderPass);
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout() {

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void RenderSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before swapchain layout");

  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  graphicsPipeline = std::make_unique<Pipeline>(
      device, "spv/shader.vert.spv", "spv/shader.frag.spv", pipelineConfig);
}

void RenderSystem::renderOrayObjects(FrameInfo &frameInfo,
                                     std::vector<OrayObject> &orayObjects) {
  graphicsPipeline->bind(frameInfo.commandBuffer);
  auto projectionView = frameInfo.camera.getProjection() * frameInfo.camera.getView();

  for (auto &obj : orayObjects) {
    SimplePushConstantData push{};
    auto modelMatrix = obj.transform.mat4();
    push.transform = projectionView * modelMatrix;
    push.normalMatrix = obj.transform.normalMatrix();

    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);
    obj.geom->bind(frameInfo.commandBuffer);
    obj.geom->draw(frameInfo.commandBuffer);
  }
}

} // namespace oray