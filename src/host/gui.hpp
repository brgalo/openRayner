#include "swapchain.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {
class Gui {
public:
  Gui(Device &device, GLFWwindow *window, SwapChain *swapchain);
  ~Gui();
  VkRenderPass getRenderPass() { return renderPass; };
  void recordImGuiCommands(VkCommandBuffer buffer, uint32_t imgIdx, VkExtent2D extent);
  void recreateFramebuffers(SwapChain *swapchain);
  Gui(const Gui &) = delete;
  Gui &operator=(const Gui &) = delete;

private:
  void createContext(VkFormat imageFormat, uint32_t imageCount);
  void createImGuiRenderPass(VkFormat imageFormat);
  void destroyImGuiRenderPass();
  void createDescriptorPool();
  void uploadFonts();
  void createFramebuffers(SwapChain *swapchain);
  void destroyFramebuffers();
  VkRenderPass renderPass;
  Device &device;
  VkDescriptorPool descriptorPool;
  std::vector<VkFramebuffer> frameBuffers;
  GLFWwindow *window;
};
} // namespace oray