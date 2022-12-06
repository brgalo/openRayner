#include "app.hpp"
#include "GLFW/glfw3.h"

namespace oray {
void Application::run() {
    while(!window.shoudClose()) {
        glfwPollEvents();
    }
}
}