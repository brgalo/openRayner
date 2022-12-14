#include "app.hpp"
#include "GLFW/glfw3.h"
#include "pipeline.hpp"
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <array>

namespace oray {

Application::Application() {
  createPipelineLayout();
  createPipeline();
  createCommandBuffers();
}

Application::~Application() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void Application::run() {
  while (!window.shoudClose()) {
    glfwPollEvents();
  }
}

void Application::createPipelineLayout() {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void Application::createPipeline() {
  auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(swapchain.width(),
                                                            swapchain.height());
  pipelineConfig.renderPass = swapchain.getRenderPass();
  pipelineConfig.pipelineLayout = pipelineLayout;
  graphicsPipeline = std::make_unique<Pipeline>(device, "spv/shader.vert.spv",
                                                "spv/shader.frag.spv",
                                                pipelineConfig);
}
void Application::createCommandBuffers() {
  commandBuffers.resize(swapchain.imageCount());
  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  for (size_t i = 0; i < commandBuffers.size(); ++i) {
    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = swapchain.getRenderPass();
    renderPassInfo.framebuffer = swapchain.getFrameBuffer(i);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    
  }
  };
  void Application::drawFrame() {};
} // namespace oray 