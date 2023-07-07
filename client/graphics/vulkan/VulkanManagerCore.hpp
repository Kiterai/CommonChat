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

    void render(uint32_t targetIndex, uint32_t imageIndex,
                std::initializer_list<vk::Semaphore> waitSemaphores,
                std::initializer_list<vk::PipelineStageFlags> waitStages,
                std::initializer_list<vk::Semaphore> signalSemaphores,
                vk::Fence fence);
};
