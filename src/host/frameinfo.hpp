#pragma once

#include "camera.hpp"

#include <vulkan/vulkan.h>
namespace oray {
struct FrameInfo {
  int frameIndex;
  float frameTime;
  VkCommandBuffer commandBuffer;
  Camera &camera;
  VkDescriptorSet globalDescriptorSet;
};
}