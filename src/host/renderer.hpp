#pragma once

#include "geometry.hpp"
#include "gui.hpp"
#include "window.hpp"

#include <cassert>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {
class Renderer {
public:
  Renderer(Window &win, Device &dev, State &state);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  VkRenderPass getSwapchainTriangleRenderpass() const {
    return swapchain->getTriangleRenderPass();
  };
  VkRenderPass getSwapchainLineRenderPass() const {
    return swapchain->getLineRenderPass();
  };
  float getAspectRatio() const { return swapchain->extentAspectRatio(); };
  bool isFrameInProgress() const { return isFrameStarted; };

  VkCommandBuffer getCurrentCommandBuffer() const {
    assert(isFrameStarted &&
           "cannot get command buffer when frame is not started!");
    return commandBuffers[currentFrameIdx];
  }

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginSwapchainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapchainRenderPass(VkCommandBuffer commandBuffer);
  void renderGui(VkCommandBuffer commandBuffer) {
    gui->recordImGuiCommands(commandBuffer, currentImgIdx,
                             swapchain->getSwapChainExtent());
  }

  int getFrameIndex() const {
    assert(isFrameStarted &&
           "Cannot get frame index when frame not in progress!");
    return currentFrameIdx;
  }

private:
  void createCommandBuffers();
  void freeCommandBuffers();
  void recreateSwapchain();

  Window &window;
  Device &device;
  State &state;
  std::unique_ptr<SwapChain> swapchain;
  std::unique_ptr<Gui> gui;
  std::vector<VkCommandBuffer> commandBuffers;

  uint32_t currentImgIdx;
  int currentFrameIdx{0};
  bool isFrameStarted{false};
};

} // namespace oray