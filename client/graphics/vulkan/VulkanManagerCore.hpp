#include "Helper.hpp"
#include <vulkan/vulkan.hpp>

class VulkanManagerCore {
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    UsingQueueSet queueSet;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::UniqueCommandPool cmdPool;
    vk::UniqueCommandBuffer cmdBuf;
    vk::UniquePipelineLayout pipelinelayout;
    std::vector<RenderTarget> renderTargets;

  public:
    VulkanManagerCore(
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        const UsingQueueSet &queueSet,
        vk::Device device);

    void recreateRenderTarget(std::vector<RenderTargetHint> hints);
};
