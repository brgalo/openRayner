#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "device.hpp"
#include <vulkan/vulkan_core.h>

namespace oray {
class Gui {
public:
  Gui(Device &device);

private:
  Device &device;
  VkDescriptorPool descriptorPool;
};
}