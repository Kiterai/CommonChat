#ifdef USE_DESKTOP_MODE

#include "../IGraphics.hpp"
#include "Helper.hpp"
#include "VulkanManagerCore.hpp"
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.hpp>

class VulkanManagerGlfw : public IGraphics {
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    UsingQueueSet queueSet;
    vk::UniqueDevice device;
    vk::Queue presentQueue;

    VulkanManagerCore core;

    SwapchainDetails swapchain;

    uint32_t flightFramesNum = 2, flightFrameIndex = 0;
    std::vector<vk::UniqueSemaphore> imageAcquiredSemaphores;

  public:
    VulkanManagerGlfw(GLFWwindow *window);

    void buildRenderTarget();

    void render();
};

#endif
