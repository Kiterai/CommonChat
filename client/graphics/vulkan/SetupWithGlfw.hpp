#ifdef USE_DESKTOP_MODE

#include "Helper.hpp"
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.hpp>

vk::UniqueInstance createVulkanInstanceWithGlfw();
vk::UniqueSurfaceKHR createVulkanSurfaceWithGlfw(const vk::Instance instance, GLFWwindow *window);
vk::UniqueDevice createVulkanDeviceWithGlfw(vk::PhysicalDevice physicalDevice, const UsingQueueSet &queueSet);
vk::PhysicalDevice chooseSuitablePhysicalDeviceWithGlfw(vk::Instance instance, vk::SurfaceKHR surface);
SwapchainDetails createVulkanSwapchainWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface);
std::vector<RenderTargetHint> getRenderTargetHintsWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, const SwapchainDetails &swapchain);

#endif
