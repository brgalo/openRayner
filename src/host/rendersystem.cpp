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
  alignas(16) glm::vec3 color;
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

void RenderSystem::renderOrayObjects(VkCommandBuffer commandBuffer,
                                     std::vector<OrayObject> &orayObjects) {
  graphicsPipeline->bind(commandBuffer);

  for (auto &obj : orayObjects) {
    obj.transform.rotation.y =
        glm::mod(obj.transform.rotation.y + 0.0001f, glm::two_pi<float>());
    obj.transform.rotation.x =
        glm::mod(obj.transform.rotation.x + 0.00005f, glm::two_pi<float>());
    
    SimplePushConstantData push{};
    push.color = obj.color;
    push.transform = obj.transform.mat4();
    vkCmdPushConstants(commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | 
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);
    obj.geom->bind(commandBuffer);
    obj.geom->draw(commandBuffer);
  }
}

} // namespace oray