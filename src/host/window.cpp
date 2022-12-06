#include "window.hpp"

namespace oray {
Window::Window(int w, int h, std::string name) : width{w},height{h},windowName{name} {
    initWindow();
}

void Window::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
}