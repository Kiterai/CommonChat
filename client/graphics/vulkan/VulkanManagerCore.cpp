#include "VulkanManagerCore.hpp"
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/SimpleRenderProc.hpp"
using namespace std::string_literals;

constexpr uint32_t renderCmdBufNum = 8;
constexpr uint32_t coreflightFramesNum = 2;

std::vector<SimpleVertex> vertices = {{0.5, 0.0, 0.0}, {-0.5, 0.0, 0.0}, {0.0, 0.0, 0.5}};
std::vector<uint32_t> indices = {0, 1, 2};

struct SceneData {
    glm::mat4x4 view;
    glm::mat4x4 proj;
};

vk::UniqueDescriptorPool createDescPool(vk::Device device) {
    vk::DescriptorPoolCreateInfo createInfo;
    vk::DescriptorPoolSize poolSizes[4];
    poolSizes[0].descriptorCount = 8;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[1].descriptorCount = 8;
    poolSizes[1].type = vk::DescriptorType::eUniformBufferDynamic;
    poolSizes[2].descriptorCount = 8;
    poolSizes[2].type = vk::DescriptorType::eSampledImage;
    poolSizes[3].descriptorCount = 8;
    poolSizes[3].type = vk::DescriptorType::eStorageBuffer;

    createInfo.maxSets = 16;
    createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    createInfo.poolSizeCount = std::size(poolSizes);
    createInfo.pPoolSizes = poolSizes;
    return device.createDescriptorPoolUnique(createInfo);
}

vk::UniqueDescriptorSetLayout createDescLayout(vk::Device device) {
    vk::DescriptorSetLayoutBinding binding[1];
    binding[0].binding = 0;
    binding[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.bindingCount = std::size(binding);
    createInfo.pBindings = binding;

    return device.createDescriptorSetLayoutUnique(createInfo);
}

std::vector<vk::UniqueDescriptorSet> createDescSets(vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout, uint32_t n) {
    std::vector<vk::DescriptorSetLayout> layouts(n, layout);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = pool;
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.descriptorSetCount = layouts.size();
    return device.allocateDescriptorSetsUnique(allocInfo);
}

RenderTarget createRenderTargetFromHint(vk::Device device, const RenderTargetHint hint, vk::PipelineLayout pipelinelayout) {
    RenderTarget rt;

    rt.extent = hint.extent;
    rt.format = hint.format;
    rt.imageViews = createImageViewsFromImages(device, hint.images, hint.format);
    return rt;
}

VulkanManagerCore::VulkanManagerCore(
    vk::Instance instance,
    vk::PhysicalDevice physicalDevice,
    const UsingQueueSet &queueSet,
    vk::Device device)
    : instance{instance},
      physicalDevice{physicalDevice},
      queueSet{queueSet},
      device{device},
      graphicsQueue{device.getQueue(queueSet.graphicsQueueFamilyIndex, 0)},
      renderCmdPool{createCommandPool(device, queueSet.graphicsQueueFamilyIndex)},
      renderCmdBufs{createCommandBuffers(device, renderCmdPool.get(), renderCmdBufNum)},
      renderCmdBufFences{createFences(device, renderCmdBufNum, true)},
      descPool{createDescPool(device)},
      descLayout{createDescLayout(device)},
      descSets{createDescSets(device, descPool.get(), descLayout.get(), coreflightFramesNum)},
      defaultRenderProc{new SimpleRenderProc{device, descLayout.get()}},
      assetManageCmdBuf{createCommandBuffer(device, renderCmdPool.get())},
      assetManageFence{std::move(createFences(device, 1, true)[0])} {

    modelVertBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                            static_cast<void *>(vertices.data()), vertices.size() * sizeof(SimpleVertex),
                            vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelIndexBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                             indices.data(), indices.size() * sizeof(uint32_t),
                             vk::BufferUsageFlagBits::eIndexBuffer, assetManageFence.get());

    for (uint32_t i = 0; i < coreflightFramesNum; i++) {
        uniformBuffer.emplace_back(physicalDevice, device, sizeof(SceneData), vk::BufferUsageFlagBits::eUniformBuffer);
    }

    for (uint32_t i = 0; i < coreflightFramesNum; i++) {
        vk::WriteDescriptorSet writeDescSet;
        writeDescSet.dstSet = descSets[i].get();
        writeDescSet.dstBinding = 0;
        writeDescSet.dstArrayElement = 0;
        writeDescSet.descriptorType = vk::DescriptorType::eUniformBuffer;

        vk::DescriptorBufferInfo descBufInfo[1];
        descBufInfo[0].buffer = uniformBuffer[i].getBuffer();
        descBufInfo[0].offset = 0;
        descBufInfo[0].range = sizeof(SceneData);

        writeDescSet.descriptorCount = std::size(descBufInfo);
        writeDescSet.pBufferInfo = descBufInfo;

        SceneData *dat = static_cast<SceneData *>(uniformBuffer[i].get());
        dat->view = glm::lookAt(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        dat->proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10.0f);

        device.updateDescriptorSets({writeDescSet}, {});
    }
}

VulkanManagerCore::~VulkanManagerCore() {
    graphicsQueue.waitIdle();
}

void VulkanManagerCore::recreateRenderTarget(std::vector<RenderTargetHint> hints) {
    rprtd.clear();
    renderTargets.clear();
    std::transform(hints.begin(), hints.end(), std::back_inserter(renderTargets),
                   [this](const RenderTargetHint &hint) {
                       return createRenderTargetFromHint(device, hint, pipelinelayout.get());
                   });
    std::transform(renderTargets.begin(), renderTargets.end(), std::back_inserter(rprtd),
                   [this](const RenderTarget &rt) {
                       return defaultRenderProc->prepareRenderTargetDependant(rt);
                   });
}

vk::Fence VulkanManagerCore::render(uint32_t targetIndex, uint32_t imageIndex,
                                    std::initializer_list<vk::Semaphore> waitSemaphores,
                                    std::initializer_list<vk::PipelineStageFlags> waitStages,
                                    std::initializer_list<vk::Semaphore> signalSemaphores) {
    auto currentFence = renderCmdBufFences[renderCmdBufIndex].get();
    auto currentCmdBuf = renderCmdBufs[renderCmdBufIndex].get();
    auto currentDescSet = descSets[flightIndex].get();
    renderCmdBufIndex = (renderCmdBufIndex + 1) % renderCmdBufNum;
    flightIndex = (flightIndex + 1) % coreflightFramesNum;

    device.waitForFences({currentFence}, true, UINT64_MAX);
    device.resetFences({currentFence});

    RenderDetails rd;
    rd.cmdBuf = currentCmdBuf;
    rd.vertBuf = modelVertBuffer.value().getBuffer();
    rd.indexBuf = modelIndexBuffer.value().getBuffer();
    rd.descSet = currentDescSet;
    rd.modelsCount = 1;
    rd.imageIndex = imageIndex;
    // rd.drawBuf =

    defaultRenderProc->render(rd, renderTargets[targetIndex], rprtd[targetIndex]);
    auto submitCmdBufs = {currentCmdBuf};

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = submitCmdBufs.size();
    submitInfo.pCommandBuffers = submitCmdBufs.begin();

    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.begin();
    submitInfo.pWaitDstStageMask = waitStages.begin();

    submitInfo.signalSemaphoreCount = signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.begin();

    graphicsQueue.submit({submitInfo}, currentFence);

    return currentFence;
}
