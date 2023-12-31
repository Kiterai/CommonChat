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

std::optional<UsingQueueSet> chooseSuitableQueueSet(const std::vector<vk::QueueFamilyProperties> queueProps);
vk::UniqueImageView createImageViewFromImage(vk::Device device, const vk::Image &image, vk::Format format, uint32_t arrayNum, vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor);
std::vector<vk::UniqueImageView> createImageViewsFromImages(vk::Device device, const std::vector<vk::Image> &images, vk::Format format, vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor);
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

std::optional<uint32_t> findMemoryTypeIndex(vk::PhysicalDevice physDevice, std::optional<vk::MemoryPropertyFlags> memFlagReq, std::optional<vk::MemoryRequirements> memReq);
void writeByMemoryMapping(vk::Device device, vk::DeviceMemory memory, const void *src, size_t sz, vk::DeviceSize dstOffset);
void writeByBufferCopy(vk::Device device, vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Buffer srcBuf, vk::Buffer dstBuf, vk::DeviceSize sz, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset, vk::Fence fence);
void writeByBufferToImageCopy(vk::Device device, vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Buffer srcBuf, vk::Image dstImg, vk::Extent3D extent, uint32_t arrayNum, vk::DeviceSize srcOffset, vk::Fence fence);
