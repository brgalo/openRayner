#include "app.hpp"

#include "rendersystem.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "GLFW/glfw3.h"
#include "geometry.hpp"
#include "pipeline.hpp"
#include <array>
#include <cassert>
#include <stdexcept>


namespace oray {

Application::Application() {
  loadOrayObjects();
}

Application::~Application() {
}

void Application::run() {
  RenderSystem renderSystem{device, renderer.getSwapchainRenderpass()};

  while (!window.shoudClose()) {
    glfwPollEvents();

    if (auto commandBuffer = renderer.beginFrame()) {
      renderer.beginSwapchainRenderPass(commandBuffer);
      renderSystem.renderOrayObjects(commandBuffer, orayObjects);
      renderer.endSwapchainRenderPass(commandBuffer);
      renderer.endFrame();
    }
  }

  vkDeviceWaitIdle(device.device());
}

void Application::loadOrayObjects() {
  std::vector<Geometry::Vertex> vertices{{{0.0f, -.5f}, {1.0f, 0.0f, 0.0f}},
                                         {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                         {{-.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
  auto geometry = std::make_shared<Geometry>(device, vertices);

  auto triangle = OrayObject::createOrayObject();
  triangle.geom = geometry;
  triangle.color = {.1f, 0.8f, 0.1f};

  triangle.transform2d.translation.x = 0.2f;
  triangle.transform2d.scale = {2.0f, 0.5f};
  triangle.transform2d.rotation = 0.25f * glm::two_pi<float>();
  orayObjects.push_back(std::move(triangle));
}

} // namespace oray