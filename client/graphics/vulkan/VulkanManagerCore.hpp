#include "Helper.hpp"
#include <vulkan/vulkan.hpp>

class VulkanManagerCore {
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    UsingQueueSet queueSet;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::UniqueCommandPool renderCmdPool;
    std::vector<vk::UniqueCommandBuffer> renderCmdBufs;
    std::vector<vk::UniqueFence> renderCmdBufFences;
    uint32_t renderCmdBufIndex = 0;
    vk::UniquePipelineLayout pipelinelayout;
    std::vector<RenderTarget> renderTargets;

  public:
    VulkanManagerCore(
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        const UsingQueueSet &queueSet,
        vk::Device device);

    void recreateRenderTarget(std::vector<RenderTargetHint> hints);

    vk::Fence render(uint32_t targetIndex, uint32_t imageIndex,
                     std::initializer_list<vk::Semaphore> waitSemaphores,
                     std::initializer_list<vk::PipelineStageFlags> waitStages,
                     std::initializer_list<vk::Semaphore> signalSemaphores);
};
