#pragma once

#include "window.hpp"
#include "geometry.hpp"
#include "orayobject.hpp"
#include "renderer.hpp"

#include <memory>
#include <vector>

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
  void loadOrayObjects();

  Window window{WIDTH, HEIGHT, "Hello VLKN!"};
  Device device{window};
  Renderer renderer{window, device};
  std::vector<OrayObject> orayObjects;
};

} // namespace oray