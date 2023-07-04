
#ifdef USE_DESKTOP_MODE

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include "Helper.hpp"

vk::UniqueInstance createVulkanInstanceWithGlfw();
vk::UniqueSurfaceKHR createVulkanSurfaceWithGlfw(const vk::Instance instance, GLFWwindow *window);
vk::UniqueDevice createVulkanDeviceWithGlfw(vk::PhysicalDevice physicalDevice);
vk::PhysicalDevice chooseSuitablePhysicalDeviceWithGlfw(vk::Instance instance, vk::SurfaceKHR surface);
Swapchain createVulkanSwapchainWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface);
std::vector<RenderTarget> createRenderTargetsWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface);

#endif
