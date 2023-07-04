#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
#include <filesystem>

struct UsingQueueSet {
    uint32_t graphicsQueueFamilyIndex;
};

struct Swapchain {
    vk::Format format;
    vk::Extent2D extent;
    vk::UniqueSwapchainKHR swapchain;
};

struct RenderTarget {
    Swapchain swapchain;
    std::vector<vk::UniqueImageView> imageViews;
    std::vector<vk::UniqueFramebuffer> frameBufs;
};

std::optional<UsingQueueSet> chooseSuitableQueueSet(const std::vector<vk::QueueFamilyProperties> queueProps);
std::vector<vk::UniqueImageView> createImageViewsFromSwapchain(vk::Device device, const Swapchain &swapchain);
std::vector<vk::UniqueFramebuffer> createFrameBufsFromImageView(vk::Device device, vk::RenderPass renderpass, vk::Extent2D extent, const std::vector<std::reference_wrapper<const std::vector<vk::UniqueImageView>>> imageViews);

vk::UniqueShaderModule createShaderModuleFromBinary(vk::Device device, const std::vector<char> &binary);
vk::UniqueShaderModule createShaderModuleFromFile(vk::Device device, const std::filesystem::path &path);
