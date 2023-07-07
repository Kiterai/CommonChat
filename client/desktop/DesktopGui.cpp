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

    graphicManager = std::make_unique<VulkanManagerGlfw>(window);
    graphicManager->buildRenderTarget();
}

DesktopGuiSystem::~DesktopGuiSystem() {
    glfwTerminate();
}

void DesktopGuiSystem::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        graphicManager->render();
    }
}

#endif