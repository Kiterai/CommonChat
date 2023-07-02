#ifdef USE_DESKTOP_MODE

#include "DesktopGui.hpp"
#include "GLFWHelper.hpp"

DesktopGuiSystem::DesktopGuiSystem() {}

DesktopGuiSystem::~DesktopGuiSystem() {}

void DesktopGuiSystem::mainLoop() {
    if (!glfwInit())
        __GLFW_ERROR_THROW

    auto window = glfwCreateWindow(1280, 720, "CommonChat", NULL, NULL);
    if (!window)
        __GLFW_TERMINATE_ERROR_THROW

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();
}

#endif