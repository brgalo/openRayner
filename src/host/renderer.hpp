#pragma once

#include "swapchain.hpp"
#include "window.hpp"
#include "geometry.hpp"

#include <cassert>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {
class Renderer {
public:
  Renderer(Window &win, Device &dev);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  VkRenderPass getSwapchainRenderpass() const {
    return swapchain->getRenderPass();
  };
  float getAspectRatio() const {return swapchain->extentAspectRatio();};
  bool isFrameInProgress() const {return isFrameStarted;};

  VkCommandBuffer getCurrentCommandBuffer() const {
    assert(isFrameStarted && "cannot get command buffer when frame is not started!");
    return commandBuffers[currentFrameIdx];
  }
  
  VkCommandBuffer beginFrame();
  void endFrame();
  void beginSwapchainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapchainRenderPass(VkCommandBuffer commandBuffer);

  int getFrameIndex() const {
    assert(isFrameStarted &&
           "Cannot get frame index when frame not in progress!");
    return currentFrameIdx;
  }

private:
  void createCommandBuffers();
  void freeCommandBuffers();
  void drawFrame();
  void recreateSwapchain();

  Window& window;
  Device& device;
  std::unique_ptr<SwapChain> swapchain;
  std::vector<VkCommandBuffer> commandBuffers;

  uint32_t currentImgIdx;
  int currentFrameIdx{0};
  bool isFrameStarted{false};
};

} // namespace oray