#include "Helper.hpp"
#include <optional>
#include <vulkan/vulkan.hpp>
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>

struct OpenxrSwapchainDetails {
    xr::Swapchain swapchain;
    int64_t format;
    xr::Extent2Di extent;
};

vk::Instance createVulkanInstanceWithOpenxr(xr::Instance xrInstance, xr::SystemId xrSystemId);
vk::PhysicalDevice getPhysicalDeviceWithOpenxr(xr::Instance xrInstance, xr::SystemId xrSystemId, vk::Instance vkInstance);
vk::Device createDeviceWithOpenxr(xr::Instance xrInstance, xr::SystemId xrSystemId, vk::PhysicalDevice physicalDevice);
std::optional<int64_t> chooseXrSwapchainFormat(const std::vector<int64_t> formats);
std::vector<vk::Image> getImagesFromXrSwapchain(xr::Swapchain swapchain);
std::vector<RenderTargetHint> getRenderTargetHintsWithOpenxr(const std::vector<OpenxrSwapchainDetails> &swapchains);
