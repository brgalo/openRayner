#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace oray {
class Window {
public:
  Window(int w, int h, std::string name);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  bool shoudClose() { return glfwWindowShouldClose(window); }
  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  };
  void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

private:
  void initWindow();

  const int width;
  const int height;

  std::string windowName;
  GLFWwindow *window = nullptr;
};
} // namespace oray