#ifdef USE_DESKTOP_MODE

#include "../IGraphics.hpp"
#include "Helper.hpp"
#include "VulkanManager.hpp"
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.hpp>

class VulkanManagerGlfw : public IGraphics {
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    UsingQueueSet queueSet;
    vk::UniqueDevice device;

    VulkanManagerCore core;

    SwapchainDetails swapchain;

  public:
    VulkanManagerGlfw(GLFWwindow *window);

    void buildRenderTarget();
};

pIGraphics makeFromDesktopGui_Vulkan(GLFWwindow *window);

#endif
