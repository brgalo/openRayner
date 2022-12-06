#pragma once

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

#include <string>

namespace oray {
    class Window {
        public:
        Window(int w, int h, std::string name);
        ~Window();

        bool shoudClose() {return glfwWindowShouldClose(window);}

        Window(const Window&) = delete;
        Window &operator=(const Window&) = delete;

        private:
            void initWindow();

            const int width;
            const int height;

            std::string windowName;
            GLFWwindow *window = nullptr;
    };
}