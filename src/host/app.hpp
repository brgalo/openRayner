#pragma once

#include "window.hpp"
#include "pipeline.hpp"


namespace oray {
class Application {
public:
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;
    void run();
private:
    Window window{WIDTH, HEIGHT, "Hello VLKN!"};
    Device device{window};
    Pipeline graphicsPipeline{device, "spv/shader.vert.spv", "spv/shader.frag.spv", Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
};


}