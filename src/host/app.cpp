#include "app.hpp"
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

Application::Application() {
  loadModels();
  createPipelineLayout();
  recreateSwapchain();
  createCommandBuffers();
}

Application::~Application() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void Application::run() {
  while (!window.shoudClose()) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device.device());
}

void Application::loadModels() {
  std::vector<Geometry::Vertex> vertices{{{0.0f, -.5f}, {1.0f, 0.0f, 0.0f}},
                                         {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                         {{-.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
  geometry = std::make_unique<Geometry>(device, vertices);
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
  assert(swapchain != nullptr && "Cannot create pipeline before swapchain");
  assert(pipelineLayout != nullptr && "Cannot create pipeline before swapchain layout");

  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = swapchain->getRenderPass();
  pipelineConfig.pipelineLayout = pipelineLayout;
  graphicsPipeline = std::make_unique<Pipeline>(
      device, "spv/shader.vert.spv", "spv/shader.frag.spv", pipelineConfig);
}

void Application::createCommandBuffers() {
  commandBuffers.resize(swapchain->imageCount());
  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
};

void Application::freeCommandBuffers() {
  vkFreeCommandBuffers(device.device(), device.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  commandBuffers.clear();
}

void Application::recordCommandBuffer(int imgIdx) {
  VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(commandBuffers[imgIdx], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = swapchain->getRenderPass();
    renderPassInfo.framebuffer = swapchain->getFrameBuffer(imgIdx);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[imgIdx], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->getSwapChainExtent().width);
    viewport.height =
        static_cast<float>(swapchain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapchain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffers[imgIdx], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[imgIdx], 0, 1, &scissor);

    graphicsPipeline->bind(commandBuffers[imgIdx]);
    geometry->bind(commandBuffers[imgIdx]);
    geometry->draw(commandBuffers[imgIdx]);

    vkCmdEndRenderPass(commandBuffers[imgIdx]);
    if (vkEndCommandBuffer(commandBuffers[imgIdx]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
}

void Application::recreateSwapchain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device.device());
  if (swapchain == nullptr) {
  swapchain = std::make_unique<SwapChain>(device, extent);
  } else {
    swapchain =
        std::make_unique<SwapChain>(device, extent, std::move(swapchain));
    if (swapchain->imageCount() != commandBuffers.size()) {
      freeCommandBuffers();
      createCommandBuffers();
    }
  }
  // if render pass compatible, dont do anything else
  createPipeline();
}

void Application::drawFrame() {
  uint32_t imageIndex;
  auto result = swapchain->acquireNextImage(&imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to aquire swapchain image!");
  }
  recordCommandBuffer(imageIndex);
  result =
      swapchain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    window.resetWindowResizedFlag();
    recreateSwapchain();
    return;
  }
  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
};
} // namespace oray