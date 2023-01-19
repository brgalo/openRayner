#include "rendersystem.hpp"
#include "glm/fwd.hpp"
#include "orayobject.hpp"
#include "raytracing.hpp"

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
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
};

RenderSystem::RenderSystem(Device &device, VkRenderPass triRenderPass,
                           VkRenderPass lineRenderPass,
                           VkDescriptorSetLayout globalSetLayout)
    : device{device} {
  // create pipelinelayout for both pipelines
  createPipelineLayouts(globalSetLayout);

  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);

  // triangle pipeline
  createPipeline(triRenderPass, trianglePipeline, trianglePipelineLayout,
                 pipelineConfig, Geometry::getBindingDescriptionsTriangle(),
                 Geometry::getAttributeDescriptionsTriangle(),
                 "spv/shader.vert.spv", "spv/shader.frag.spv");

  // change toplogy to have a triangle render pipeline
  pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  // make linewidth dynamic
  pipelineConfig.dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
  pipelineConfig.dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(pipelineConfig.dynamicStateEnables.size());
  pipelineConfig.dynamicStateInfo.pDynamicStates =
      pipelineConfig.dynamicStateEnables.data();
  pipelineConfig.rasterizationInfo.lineWidth = 1.f;
  
  createPipeline(lineRenderPass, linePipeline, linePipelineLayout,
                 pipelineConfig, Geometry::getBindingDescriptionsLine(),
                 Geometry::getAttributeDescriptionsLine(),
                 "spv/lineShader.vert.spv", "spv/lineShader.frag.spv");
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(device.device(), trianglePipelineLayout, nullptr);
  vkDestroyPipelineLayout(device.device(), linePipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayouts(
    VkDescriptorSetLayout globalSetLayout) {
  // triange
  createPipelineLayout(globalSetLayout, trianglePipelineLayout, true);
  // lines
  createPipelineLayout(globalSetLayout, linePipelineLayout);
}

void RenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout,
                                        VkPipelineLayout &pipelineLayout,
                                        bool WITH_CONSTANTS) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  if (WITH_CONSTANTS) {
    pipelineLayoutInfo.pushConstantRangeCount = 1;
  } else {
    pushConstantRange.size = sizeof(RtPushConstants);
    pipelineLayoutInfo.pushConstantRangeCount = 1;
  }

  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void RenderSystem::createPipeline(
    VkRenderPass renderPass, std::unique_ptr<Pipeline> &pipeline,
    VkPipelineLayout pipelineLayout, PipelineConfigInfo &pipelineConfig,
    const std::vector<VkVertexInputBindingDescription> &bindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions,
    const std::string vertShaderFilepath,
    const std::string fragShaderFilepath) {

  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before swapchain layout");

  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  pipeline = std::make_unique<Pipeline>(
      device, vertShaderFilepath, fragShaderFilepath, pipelineConfig,
      bindingDescriptions, attributeDescriptions);
}

void RenderSystem::renderOrayObjects(FrameInfo &frameInfo,
                                     std::vector<OrayObject> &orayObjects) {
  trianglePipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(
      frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      trianglePipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

  for (auto &obj : orayObjects) {
    SimplePushConstantData push{};
    push.modelMatrix = obj.transform.mat4();
    push.normalMatrix = obj.transform.normalMatrix();
    vkCmdPushConstants(frameInfo.commandBuffer, trianglePipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);
    obj.geom->bind(frameInfo.commandBuffer);
    obj.geom->draw(frameInfo.commandBuffer);
  }
}

void RenderSystem::renderLines(FrameInfo &frameInfo, Raytracer &rt) {
  linePipeline->bind(frameInfo.commandBuffer);
  vkCmdSetLineWidth(frameInfo.commandBuffer, rt.state->lineWidth);
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, linePipelineLayout,
                          0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
  vkCmdPushConstants(frameInfo.commandBuffer, linePipelineLayout,
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(RtPushConstants), rt.pushConsts());
  vkCmdDraw(frameInfo.commandBuffer, rt.state->nBufferElements * 2, 1, 0, 0);
}

} // namespace oray