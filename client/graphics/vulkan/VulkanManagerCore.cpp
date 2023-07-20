#include "VulkanManagerCore.hpp"
#include "renderer/SimpleRenderProc.hpp"
#include <fastgltf/parser.hpp>
#include <future>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
using namespace std::string_literals;

constexpr uint32_t coreflightFramesNum = 2;

struct ObjectData {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 joints[32];
};

constexpr auto idmat = glm::identity<glm::mat4x4>();

std::vector<glm::vec3> posVertices = {{-0.5, 0.0, 0.0}, {0.5, 0.0, 0.0}, {-0.5, 0.5, 0.0}, {0.5, 0.5, 0.0}, {-0.5, 1.0, 0.0}, {0.5, 1.0, 0.0}, {-0.5, 1.5, 0.0}, {0.5, 1.5, 0.0}, {-0.5, 2.0, 0.0}, {0.5, 2.0, 0.0}};
std::vector<glm::vec3> normVertices = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
std::vector<glm::vec2> texcoordVertices = {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
std::vector<glm::i16vec4> jointsVertices = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}};
std::vector<glm::vec4> weightsVertices = {{1.00, 0.00, 0.0, 0.0}, {1.00, 0.00, 0.0, 0.0}, {0.75, 0.25, 0.0, 0.0}, {0.75, 0.25, 0.0, 0.0}, {0.50, 0.50, 0.0, 0.0}, {0.50, 0.50, 0.0, 0.0}, {0.25, 0.75, 0.0, 0.0}, {0.25, 0.75, 0.0, 0.0}, {0.00, 1.00, 0.0, 0.0}, {0.00, 1.00, 0.0, 0.0}};
std::vector<uint32_t> indices = {0, 1, 3, 0, 3, 2, 2, 3, 5, 2, 5, 4, 4, 5, 7, 4, 7, 6, 6, 7, 9, 6, 9, 8};
std::vector<vk::DrawIndexedIndirectCommand> indirectDraws = {{24, 1, 0, 0, 0}};
std::vector<ObjectData> objects = {
    {glm::translate(idmat, glm::vec3(0, 0, 0)),
     {
         idmat,
         glm::translate(idmat, glm::vec3(0, 1, 0)) *
             glm::rotate(idmat, glm::half_pi<float>(), glm::vec3(0, 0, 1)) *
             glm::translate(idmat, glm::vec3(0, -1, 0)),
     }}};

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
    vk::DescriptorSetLayoutBinding binding[3];
    binding[0].binding = 0;
    binding[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
    binding[1].binding = 1;
    binding[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    binding[1].descriptorCount = 1;
    binding[1].stageFlags = vk::ShaderStageFlagBits::eFragment;
    binding[2].binding = 2;
    binding[2].descriptorType = vk::DescriptorType::eStorageBuffer;
    binding[2].descriptorCount = 1;
    binding[2].stageFlags = vk::ShaderStageFlagBits::eVertex;

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
      renderCmdBufs{createCommandBuffers(device, renderCmdPool.get(), coreflightFramesNum)},
      renderCmdBufFences{createFences(device, coreflightFramesNum, true)},
      descPool{createDescPool(device)},
      descLayout{createDescLayout(device)},
      descSets{createDescSets(device, descPool.get(), descLayout.get(), coreflightFramesNum)},
      defaultRenderProc{new SimpleRenderProc{device, descLayout.get()}},
      assetManageCmdBuf{createCommandBuffer(device, renderCmdPool.get())},
      assetManageFence{std::move(createFences(device, 1, true)[0])} {

    modelPosVertBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                               static_cast<void *>(posVertices.data()), posVertices.size() * sizeof(glm::vec3),
                               vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelNormVertBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                                static_cast<void *>(normVertices.data()), normVertices.size() * sizeof(glm::vec3),
                                vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelTexcoordVertBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                                    static_cast<void *>(texcoordVertices.data()), texcoordVertices.size() * sizeof(glm::vec2),
                                    vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelJointsVertBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                                  static_cast<void *>(jointsVertices.data()), jointsVertices.size() * sizeof(glm::i16vec4),
                                  vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelWeightsVertBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                                   static_cast<void *>(weightsVertices.data()), weightsVertices.size() * sizeof(glm::vec4),
                                   vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelIndexBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                             indices.data(), indices.size() * sizeof(uint32_t),
                             vk::BufferUsageFlagBits::eIndexBuffer, assetManageFence.get());
    drawIndirectBuffer.emplace(physicalDevice, device,
                               sizeof(vk::DrawIndexedIndirectCommand) * indirectDraws.size(),
                               vk::BufferUsageFlagBits::eIndirectBuffer);
    std::copy(indirectDraws.begin(), indirectDraws.end(), static_cast<vk::DrawIndexedIndirectCommand *>(drawIndirectBuffer.value().get()));
    drawIndirectBuffer.value().flush<1>(device, {{{0, sizeof(vk::DrawIndexedIndirectCommand) * indirectDraws.size()}}});

    for (uint32_t i = 0; i < coreflightFramesNum; i++) {
        uniformBuffer.emplace_back(physicalDevice, device, sizeof(SceneData), vk::BufferUsageFlagBits::eUniformBuffer);
        objectsBuffer.emplace_back(physicalDevice, device, sizeof(ObjectData) * objects.size(), vk::BufferUsageFlagBits::eStorageBuffer);
        std::copy(objects.begin(), objects.end(), static_cast<ObjectData *>(objectsBuffer.back().get()));
    }

    for (uint32_t i = 0; i < coreflightFramesNum; i++) {
        vk::DescriptorBufferInfo descBufInfo[1];
        descBufInfo[0].buffer = uniformBuffer[i].getBuffer();
        descBufInfo[0].offset = 0;
        descBufInfo[0].range = sizeof(SceneData);

        // vk::DescriptorImageInfo descImgInfo[1];
        // // descImgInfo[0].sampler
        // descImgInfo[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        // // descImgInfo[0].imageView

        vk::DescriptorBufferInfo descStorageBufInfo[1];
        descStorageBufInfo[0].buffer = objectsBuffer[i].getBuffer();
        descStorageBufInfo[0].offset = 0;
        descStorageBufInfo[0].range = sizeof(ObjectData) * objects.size();

        vk::WriteDescriptorSet writeDescSet[2];
        writeDescSet[0].dstSet = descSets[i].get();
        writeDescSet[0].dstBinding = 0;
        writeDescSet[0].dstArrayElement = 0;
        writeDescSet[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescSet[0].descriptorCount = std::size(descBufInfo);
        writeDescSet[0].pBufferInfo = descBufInfo;
        // writeDescSet[1].dstSet = descSets[i].get();
        // writeDescSet[1].dstBinding = 1;
        // writeDescSet[1].dstArrayElement = 0;
        // writeDescSet[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        // writeDescSet[1].descriptorCount = std::size(descImgInfo);
        // writeDescSet[1].pImageInfo = descImgInfo;
        writeDescSet[1].dstSet = descSets[i].get();
        writeDescSet[1].dstBinding = 2;
        writeDescSet[1].dstArrayElement = 0;
        writeDescSet[1].descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet[1].descriptorCount = std::size(descStorageBufInfo);
        writeDescSet[1].pBufferInfo = descStorageBufInfo;

        device.updateDescriptorSets(writeDescSet, {});
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

    for (uint32_t i = 0; i < coreflightFramesNum; i++) {
        SceneData *dat = static_cast<SceneData *>(uniformBuffer[i].get());
        dat->view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        dat->proj = glm::perspective(glm::radians(45.0f), float(renderTargets[0].extent.width) / float(renderTargets[0].extent.height), 0.1f, 10.0f);
    }
}

vk::Fence VulkanManagerCore::render(uint32_t targetIndex, uint32_t imageIndex,
                                    std::initializer_list<vk::Semaphore> waitSemaphores,
                                    std::initializer_list<vk::PipelineStageFlags> waitStages,
                                    std::initializer_list<vk::Semaphore> signalSemaphores) {
    auto currentFence = renderCmdBufFences[flightIndex].get();
    auto currentCmdBuf = renderCmdBufs[flightIndex].get();
    auto currentDescSet = descSets[flightIndex].get();
    flightIndex = (flightIndex + 1) % coreflightFramesNum;

    device.waitForFences({currentFence}, true, UINT64_MAX);
    device.resetFences({currentFence});

    RenderDetails rd;
    rd.cmdBuf = currentCmdBuf;
    rd.positionVertBuf = modelPosVertBuffer.value().getBuffer();
    rd.normalVertBuf = modelNormVertBuffer.value().getBuffer();
    rd.texcoordVertBuf[0] = modelTexcoordVertBuffer.value().getBuffer();
    rd.jointsVertBuf[0] = modelJointsVertBuffer.value().getBuffer();
    rd.weightsVertBuf[0] = modelWeightsVertBuffer.value().getBuffer();
    rd.indexBuf = modelIndexBuffer.value().getBuffer();
    rd.descSet = currentDescSet;
    rd.imageIndex = imageIndex;

    rd.modelsCount = 1;
    rd.drawBuf = drawIndirectBuffer.value().getBuffer();
    rd.drawBufOffset = 0;
    rd.drawBufStride = sizeof(vk::DrawIndexedIndirectCommand);

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
