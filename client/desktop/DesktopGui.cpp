#ifdef USE_DESKTOP_MODE

#include "DesktopGui.hpp"
#include "GLFWHelper.hpp"

DesktopGuiSystem::DesktopGuiSystem() {
    if (!glfwInit())
        __GLFW_ERROR_THROW

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(1280, 720, "CommonChat", NULL, NULL);
    if (!window)
        __GLFW_ERROR_THROW
}

DesktopGuiSystem::~DesktopGuiSystem() {
    glfwTerminate();
}

void DesktopGuiSystem::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

#endif