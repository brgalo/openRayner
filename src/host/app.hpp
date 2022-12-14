#pragma once

#include "pipeline.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include "geometry.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {
class Application {
public:
  static constexpr uint32_t WIDTH = 800;
  static constexpr uint32_t HEIGHT = 600;
  void run();

  Application();
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

private:
  void loadModels();
  void createPipelineLayout();
  void createPipeline();
  void createCommandBuffers();
  void drawFrame();

  Window window{WIDTH, HEIGHT, "Hello VLKN!"};
  Device device{window};
  SwapChain swapchain{device, window.getExtent()};
  std::unique_ptr<Pipeline> graphicsPipeline;
  VkPipelineLayout pipelineLayout;
  std::vector<VkCommandBuffer> commandBuffers;
  std::unique_ptr<Geometry> geometry;
};

} // namespace oray