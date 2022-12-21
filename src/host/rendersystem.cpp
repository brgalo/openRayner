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
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
};

RenderSystem::RenderSystem(Device &device, VkRenderPass triRenderPass,
                           VkRenderPass lineRenderPass,
                           VkDescriptorSetLayout globalSetLayout)
    : device{device} {
  createPipelineLayout(globalSetLayout);
  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);

  createPipeline(triRenderPass, trianglePipeline, trianglePipelineLayout,
                 pipelineConfig, "spv/lineShader.vert.spv",
                 "spv/lineShader.frag.spv");
  pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  createPipeline(lineRenderPass, linePipeline, linePipelineLayout,
                 pipelineConfig, "spv/lineShader.vert.spv",
                 "spv/lineShader.frag.spv");
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(device.device(), trianglePipelineLayout, nullptr);
  vkDestroyPipelineLayout(device.device(), linePipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

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
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &trianglePipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create triangle pipeline layout!");
  }

  pipelineLayoutInfo.pushConstantRangeCount = 0;
  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &linePipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create triangle pipeline layout!");
  }
}

void RenderSystem::createPipeline(VkRenderPass renderPass,
                                  std::unique_ptr<Pipeline> &pipeline,
                                  VkPipelineLayout pipelineLayout,
                                  PipelineConfigInfo &pipelineConfig,
                                  const std::string vertShaderFilepath,
                                  const std::string fragShaderFilepath) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before swapchain layout");

  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  pipeline = std::make_unique<Pipeline>(device, vertShaderFilepath,
                                        fragShaderFilepath, pipelineConfig);
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

void RenderSystem::renderLines(FrameInfo &frameInfo, Buffer &lines) {
  linePipeline->bind(frameInfo.commandBuffer);
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, linePipelineLayout,
                          0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

  std::vector<VkBuffer> buffers = {lines.getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, buffers.data(),
                         offsets);
  vkCmdDraw(frameInfo.commandBuffer, 4, 1, 0, 0);
}

} // namespace oray