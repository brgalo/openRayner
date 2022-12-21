#include "renderer.hpp"
#include "GLFW/glfw3.h"
#include "geometry.hpp"
#include "pipeline.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {

Renderer::Renderer(Window &win, Device &dev) : window{win}, device{dev} {
  recreateSwapchain();
  createCommandBuffers();
}

Renderer::~Renderer() { freeCommandBuffers(); }

void Renderer::createCommandBuffers() {
  commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
};

void Renderer::freeCommandBuffers() {
  vkFreeCommandBuffers(device.device(), device.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  commandBuffers.clear();
}

void Renderer::recreateSwapchain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device.device());
  if (swapchain == nullptr) {
    swapchain = std::make_unique<SwapChain>(device, extent);
  } else {
    std::shared_ptr<SwapChain> oldSwapchain = std::move(swapchain);
    swapchain = std::make_unique<SwapChain>(device, extent, oldSwapchain);

    if (!oldSwapchain->compareSwapFormats(*swapchain.get())) {
      throw std::runtime_error(
          "swap chain image (or depth) format has changed!");
    }
  }
  // if render pass compatible, dont do anything else
  // createPipeline();
}

VkCommandBuffer Renderer::beginFrame() {
  assert(!isFrameStarted &&
         "cant't call beginFrame, while already in progress!");

  auto result = swapchain->acquireNextImage(&currentImgIdx);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return nullptr;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to aquire swapchain image!");
  }

  isFrameStarted = true;

  auto commandBuffer = getCurrentCommandBuffer();
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  return commandBuffer;
}

void Renderer::endFrame() {
  assert(isFrameStarted &&
         "can't call endFrame while frame is not in progress!");
  auto commandBuffer = getCurrentCommandBuffer();

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
  auto result = swapchain->submitCommandBuffers(&commandBuffer, &currentImgIdx);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    window.resetWindowResizedFlag();
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  isFrameStarted = false;
  currentFrameIdx = (currentFrameIdx + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginSwapchainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "can't call beginSwapchainRenderPass if frame is not in progress!");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "cant begin renderpass on a command buffer of a different frame!");

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swapchain->getTriangleRenderPass();
  renderPassInfo.framebuffer = swapchain->getFrameBuffer(currentImgIdx);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchain->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.01f, 0.01f, 0.01f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapchain->getSwapChainExtent().width);
  viewport.height = static_cast<float>(swapchain->getSwapChainExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0}, swapchain->getSwapChainExtent()};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
void Renderer::endSwapchainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "can't call endSwapchainRenderPass if frame is not in progress!");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "cant end renderpass on a command buffer of a different frame!");

  vkCmdEndRenderPass(commandBuffer);
}
} // namespace oray