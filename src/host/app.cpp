#include "app.hpp"

#include "buffer.hpp"
#include "camera.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "frameinfo.hpp"
#include "geometry.hpp"
#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "keyboard.hpp"
#include "pipeline.hpp"
#include "raytracing.hpp"
#include "rendersystem.hpp"
#include "state.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>

namespace oray {

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.2f};
  uint64_t colorBufferAddress;
};

Application::Application() {
  globalPool = DescriptorPool::Builder(device)
                   .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                SwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();
  loadOrayObjects();
  state->setTriangleNames(orayObjects->data());
  initRaytracer();

  VkCommandBuffer cmdBuf = device.beginSingleTimeCommands();
  raytracer->traceInternalVF(cmdBuf);
  device.endSingleTimeCommands(cmdBuf);
  vkDeviceWaitIdle(device.device());
}

Application::~Application() {
  // explicitly call desctructor
  //renderer.Renderer::~Renderer();
}

void Application::run() {

  std::vector<std::unique_ptr<Buffer>> uboBuffers(
      SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (auto &buffer : uboBuffers) {
    buffer = std::make_unique<Buffer>(device, sizeof(GlobalUbo), 1,
                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    buffer->map();
  }

  auto globalSetLayout = DescriptorSetLayout::Builder(device)
                             .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                         VK_SHADER_STAGE_VERTEX_BIT)
                             .build();

  std::vector<VkDescriptorSet> globalDescriptorSets(
      SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < globalDescriptorSets.size(); ++i) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    DescriptorWriter(*globalSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
  }

  RenderSystem renderSystem{device, renderer.getSwapchainTriangleRenderpass(), renderer.getSwapchainLineRenderPass(),
                            globalSetLayout->getDescriptorSetLayout()};
  Camera camera{};
  //  camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
  camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));


  auto viewerObject = OrayObject::createOrayObject();
  KeyboardController cameraController{};

  auto currentTime = std::chrono::high_resolution_clock::now();

  while (!window.shoudClose()) {
    glfwPollEvents();

    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   currentTime)
            .count();
    currentTime = newTime;
    frameTime = glm::min(frameTime, MAX_FRAME_TIME);

    cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime,
                                   viewerObject);
    camera.setViewYXZ(viewerObject.transform.translation,
                      viewerObject.transform.rotation);

    float aspect = renderer.getAspectRatio();
    //  camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
    camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1, 10);



    if (auto commandBuffer = renderer.beginFrame()) {

       if (state->doTrace && state->nRays != 0) {
      raytracer->traceTriangle(commandBuffer);
      state->doTrace = false;
      }


      int frameIndex = renderer.getFrameIndex();
      FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera,
                          globalDescriptorSets[frameIndex]};
      // update
      GlobalUbo ubo{};
      ubo.projectionView = camera.getProjection() * camera.getView();
      ubo.colorBufferAddress = raytracer->pushConsts()->hitBuffer;
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      // rendering
      renderer.beginSwapchainRenderPass(commandBuffer);
      renderSystem.renderOrayObjects(frameInfo, *orayObjects);
      if(state->nRays != 0) renderSystem.renderLines(frameInfo, *raytracer);
      renderer.endSwapchainRenderPass(commandBuffer);
      renderer.renderGui(commandBuffer);
      renderer.endFrame();
    }
  }

  vkDeviceWaitIdle(device.device());
}

// temporary helper function, creates a 1x1x1 cube centered at offset with an
// index buffer
std::unique_ptr<Geometry> createCubeModel(Device &device, glm::vec3 offset) {
  Geometry::Builder modelBuilder{};
  modelBuilder.vertices = {
      // left face (white)
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

      // right face (yellow)
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

      // top face (orange, remember y axis points down)
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},

      // bottom face (red)
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

      // nose face (blue)
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

      // tail face (green)
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
  };
  for (auto &v : modelBuilder.vertices) {
    v.position += offset;
  }

  modelBuilder.indices = {0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,
                          8,  9,  10, 8,  11, 9,  12, 13, 14, 12, 15, 13,
                          16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};

  return std::make_unique<Geometry>(device, modelBuilder);
}

void Application::loadOrayObjects() {
  std::shared_ptr<Geometry> geometry =
      Geometry::createModelFromFile(device, "models/two_plates.obj");

  auto orayObj = OrayObject::createOrayObject();
  orayObj.geom = geometry;
  orayObj.transform.translation = {.0f, .0f, .0f};
  orayObj.transform.scale = {1.f, 1.f, 1.f};
  orayObjects->push_back(std::move(orayObj));
}

void Application::initRaytracer() {
  raytracer = std::make_unique<Raytracer>(device, *orayObjects, state);
  VkCommandBuffer buf = device.beginSingleTimeCommands();
  raytracer->traceTriangle(buf);
  device.endSingleTimeCommands(buf);
  vkDeviceWaitIdle(device.device());
}

void Application::initCompute() {
  compute = std::make_unique<Compute>(device, *orayObjects);
}
} // namespace oray