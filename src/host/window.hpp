#pragma once

#define GLFW_INCLUDE_NONE 
#include <GLFW/glfw3.h>

namespace oray {
    class window {
        private:
            void initWindow();

            const int width;
            const int height;

            std::string windowName;
            GLFWwindow *window;
    }
}