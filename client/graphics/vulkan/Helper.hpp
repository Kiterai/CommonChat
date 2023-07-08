#pragma once

#include <filesystem>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

struct UsingQueueSet {
    uint32_t graphicsQueueFamilyIndex;
};

struct SwapchainDetails {
    vk::Format format;
    vk::Extent2D extent;
    vk::UniqueSwapchainKHR swapchain;
};

struct RenderTargetHint {
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Image> images;
};

struct RenderTarget {
    vk::UniqueRenderPass renderpass;
    vk::UniquePipeline pipeline;

    vk::Extent2D extent;
    std::vector<vk::UniqueImageView> imageViews;
    std::vector<vk::UniqueFramebuffer> frameBufs;
};

std::optional<UsingQueueSet> chooseSuitableQueueSet(const std::vector<vk::QueueFamilyProperties> queueProps);
std::vector<vk::UniqueImageView> createImageViewsFromImages(vk::Device device, const std::vector<vk::Image> &images, vk::Format format);
std::vector<vk::UniqueFramebuffer> createFrameBufsFromImageView(vk::Device device, vk::RenderPass renderpass, vk::Extent2D extent, const std::vector<std::reference_wrapper<const std::vector<vk::UniqueImageView>>> imageViews);

vk::UniqueShaderModule createShaderModuleFromBinary(vk::Device device, const std::vector<char> &binary);
vk::UniqueShaderModule createShaderModuleFromFile(vk::Device device, const std::filesystem::path &path);

vk::UniqueCommandPool createCommandPool(vk::Device device, uint32_t queueFamilyIndex);
std::vector<vk::UniqueCommandBuffer> createCommandBuffers(vk::Device device, vk::CommandPool pool, uint32_t n);
vk::UniqueCommandBuffer createCommandBuffer(vk::Device device, vk::CommandPool pool);
std::vector<vk::UniqueFence> createFences(vk::Device device, uint32_t n, bool signaled);
void Submit(std::initializer_list<vk::CommandBuffer> cmdBufs, vk::Queue queue, vk::Fence fence = {});

struct CommandRec {
    vk::CommandBuffer cmdBuf;
    CommandRec(vk::CommandBuffer cmdBuf) : cmdBuf(cmdBuf) {
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        cmdBuf.begin(beginInfo);
    }
    ~CommandRec() {
        cmdBuf.end();
    }
};

struct CommandExec {
    vk::CommandBuffer cmdBuf;
    vk::Queue queue;
    vk::Fence fence;
    CommandExec(vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Fence fence = {}) : cmdBuf(cmdBuf), queue(queue), fence(fence) {
        vk::CommandBufferBeginInfo beginInfo;
        cmdBuf.begin(beginInfo);
    }
    ~CommandExec() {
        cmdBuf.end();
        Submit({cmdBuf}, queue, fence);
    }
};
