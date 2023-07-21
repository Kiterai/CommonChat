#ifndef VULKAN_MANAGER_CORE_HPP
#define VULKAN_MANAGER_CORE_HPP

#include "Render.hpp"
#include "Helper.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
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
    uint32_t flightIndex = 0;

    vk::UniqueDescriptorPool descPool;
    vk::UniqueDescriptorSetLayout descLayout;
    std::vector<vk::UniqueDescriptorSet> descSets;

    vk::UniquePipelineLayout pipelinelayout;

    std::unique_ptr<IRenderProc> defaultRenderProc;
    std::vector<RenderTarget> renderTargets;
    std::vector<RenderProcRenderTargetDependant> rprtd;

    vk::UniqueCommandBuffer assetManageCmdBuf;
    vk::UniqueFence assetManageFence;
    std::optional<ReadonlyBuffer> modelPosVertBuffer;
    std::optional<ReadonlyBuffer> modelNormVertBuffer;
    std::optional<ReadonlyBuffer> modelTexcoordVertBuffer;
    std::optional<ReadonlyBuffer> modelJointsVertBuffer;
    std::optional<ReadonlyBuffer> modelWeightsVertBuffer;
    std::optional<ReadonlyBuffer> modelIndexBuffer;
    std::optional<CommunicationBuffer> drawIndirectBuffer;
    std::vector<CommunicationBuffer> uniformBuffer;
    std::vector<CommunicationBuffer> objectsBuffer;
    std::vector<CommunicationBuffer> jointsBuffer;

    std::optional<ReadonlyImage> testTexture;
    std::optional<vk::UniqueImageView> testTextureImgView;
    vk::UniqueSampler testSampler;

  public:
    VulkanManagerCore(
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        const UsingQueueSet &queueSet,
        vk::Device device);
    ~VulkanManagerCore();

    void recreateRenderTarget(std::vector<RenderTargetHint> hints);

    vk::Fence render(uint32_t targetIndex, uint32_t imageIndex,
                     std::initializer_list<vk::Semaphore> waitSemaphores,
                     std::initializer_list<vk::PipelineStageFlags> waitStages,
                     std::initializer_list<vk::Semaphore> signalSemaphores);
};

#endif VULKAN_MANAGER_CORE_HPP
