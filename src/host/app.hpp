#pragma once

#include "descriptors.hpp"
#include "geometry.hpp"
#include "orayobject.hpp"
#include "renderer.hpp"
#include "window.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace oray {
class Application {
public:
  static constexpr uint32_t WIDTH = 800;
  static constexpr uint32_t HEIGHT = 600;
  static constexpr float MAX_FRAME_TIME = 0.2;

  void run();

  Application();
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

private:
  void loadOrayObjects();
  State state;
  Window window{WIDTH, HEIGHT, "Hello VLKN!"};
  Device device{window};
  Renderer renderer{window, device, state};

  std::unique_ptr<DescriptorPool> globalPool{};
  std::vector<OrayObject> orayObjects;
};

} // namespace oray