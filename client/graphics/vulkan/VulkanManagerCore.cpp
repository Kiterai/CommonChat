#include "VulkanManagerCore.hpp"
#include "renderer/SimpleRenderProc.hpp"
#include <fastgltf/parser.hpp>
#include <future>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <stb_image.h>
using namespace std::string_literals;

constexpr uint32_t coreflightFramesNum = 2;

struct ObjectData {
    glm::mat4 modelMat;
    glm::uint32_t jointIndex;
    glm::uint32_t dummy[3];
};

struct MeshData {
    glm::uint32_t objectIndex;
    glm::uint32_t materialIndex;
    glm::uint32_t textureIndex; // no longer used
    glm::uint32_t dummy[1];
};

constexpr auto idmat = glm::identity<glm::mat4x4>();

std::vector<vk::DrawIndexedIndirectCommand> indirectDraws = {};
std::vector<MeshData> meshes = {};
std::vector<ObjectData> objects = {};
std::vector<glm::mat4> joints = {};

vk::UniqueDescriptorPool createDescPool(vk::Device device) {
    vk::DescriptorPoolCreateInfo createInfo;
    vk::DescriptorPoolSize poolSizes[4];
    poolSizes[0].descriptorCount = 8;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[1].descriptorCount = 8;
    poolSizes[1].type = vk::DescriptorType::eUniformBufferDynamic;
    poolSizes[2].descriptorCount = 256;
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
    vk::DescriptorSetLayoutBinding binding[4];
    // Uniform Buffer
    binding[0].binding = 0;
    binding[0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
    // Objects
    binding[1].binding = 2;
    binding[1].descriptorType = vk::DescriptorType::eStorageBufferDynamic;
    binding[1].descriptorCount = 1;
    binding[1].stageFlags = vk::ShaderStageFlagBits::eVertex;
    // Joints
    binding[2].binding = 3;
    binding[2].descriptorType = vk::DescriptorType::eStorageBufferDynamic;
    binding[2].descriptorCount = 1;
    binding[2].stageFlags = vk::ShaderStageFlagBits::eVertex;
    // Primitives
    binding[3].binding = 4;
    binding[3].descriptorType = vk::DescriptorType::eStorageBufferDynamic;
    binding[3].descriptorCount = 1;
    binding[3].stageFlags = vk::ShaderStageFlagBits::eVertex;

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

RenderTarget createRenderTargetFromHint(vk::Device device, const RenderTargetHint hint) {
    RenderTarget rt;

    rt.extent = hint.extent;
    rt.format = hint.format;
    rt.imageViews = createImageViewsFromImages(device, hint.images, hint.format);
    return rt;
}

vk::UniqueSampler createSampler(vk::Device device) {
    vk::SamplerCreateInfo createInfo;
    createInfo.magFilter = vk::Filter::eLinear;
    createInfo.minFilter = vk::Filter::eLinear;
    createInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    createInfo.anisotropyEnable = false;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = false;
    createInfo.compareEnable = false;
    createInfo.compareOp = vk::CompareOp::eAlways;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;

    return device.createSamplerUnique(createInfo);
}

struct JointConfiguration {
    glm::quat rotation;
    glm::vec3 translation;
};

void updateJointMatrix(const ModelManager::ModelInfo &model, const std::vector<JointConfiguration> &jointConfig, uint32_t indexBase) {
    std::vector<int> indices;
    std::vector<int> chCount(model.nodes.size(), 0);
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        if (model.nodes[i].parent != -1)
            chCount[model.nodes[i].parent]++;
    }
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        if (chCount[i] == 0)
            indices.push_back(i);
    }
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        int p = model.nodes[indices[i]].parent;
        if (p != -1) {
            chCount[p]--;
            if (chCount[p] == 0)
                indices.push_back(p);
        }
    }
    std::reverse(indices.begin(), indices.end());
    for (const auto i : indices) {
        joints[indexBase + i] =
            (model.nodes[i].parent == -1 ? idmat : joints[model.nodes[i].parent]) * glm::translate(idmat, jointConfig[i].translation) * glm::toMat4(jointConfig[i].rotation);
    }
    for (uint32_t i = 0; i < model.nodes.size(); i++) {
        joints[indexBase + i] *= model.nodes[i].inverseBindMatrix;
    }
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
      descSet{std::move(createDescSets(device, descPool.get(), descLayout.get(), 1)[0])},
      assetManageCmdBuf{createCommandBuffer(device, renderCmdPool.get())},
      assetManageFence{std::move(createFences(device, 1, true)[0])},
      modelManager{physicalDevice, device, descPool.get(), graphicsQueue, assetManageCmdBuf.get(), assetManageFence.get()},
      defaultRenderProc{new SimpleRenderProc{physicalDevice, device, descLayout.get(), modelManager.getDescSetLayout()}} {

    auto modelInfo = modelManager.loadModelFromGlbFile("AliciaSolid.vrm", graphicsQueue, assetManageCmdBuf.get(), assetManageFence.get());

    {
        ObjectData obj;
        obj.modelMat = glm::translate(idmat, glm::vec3{0.0, -0.5, 0.0});
        obj.jointIndex = 0;
        objects.push_back(obj);
    }
    for (const auto &primitive : modelInfo.primitives) {
        vk::DrawIndexedIndirectCommand drawCmd;
        drawCmd.vertexOffset = primitive.vertexBase;
        drawCmd.firstIndex = primitive.IndexBase;
        drawCmd.firstInstance = indirectDraws.size();
        drawCmd.instanceCount = 1;
        drawCmd.indexCount = primitive.indexNum;
        indirectDraws.push_back(drawCmd);

        MeshData mesh;
        mesh.objectIndex = 0;
        mesh.materialIndex = primitive.materialIndex;
        mesh.textureIndex = primitive.textureIndex;
        meshes.push_back(mesh);
    }
    joints.resize(modelInfo.nodes.size(), glm::identity<glm::mat4>());

    // for alignment
    objects.resize(2048);
    meshes.resize(65536);
    joints.resize(65536);

    std::vector<JointConfiguration> jointConfig(modelInfo.nodes.size());
    for (int i = 0; i < modelInfo.nodes.size(); i++) {
        jointConfig[i].rotation = modelInfo.nodes[i].rotation;
        jointConfig[i].translation = modelInfo.nodes[i].translation;
    }
    jointConfig[51].rotation = glm::quat(sqrt(0.5f), 0, -sqrt(0.5f), 0);
    updateJointMatrix(modelInfo, jointConfig, 0);

    drawIndirectBuffer.emplace(physicalDevice, device, sizeof(vk::DrawIndexedIndirectCommand) * indirectDraws.size() * coreflightFramesNum, vk::BufferUsageFlagBits::eIndirectBuffer);
    meshesBuffer.emplace(physicalDevice, device, sizeof(MeshData) * meshes.size() * coreflightFramesNum, vk::BufferUsageFlagBits::eStorageBuffer);
    objectsBuffer.emplace(physicalDevice, device, sizeof(ObjectData) * objects.size() * coreflightFramesNum, vk::BufferUsageFlagBits::eStorageBuffer);
    jointsBuffer.emplace(physicalDevice, device, sizeof(glm::mat4) * joints.size() * coreflightFramesNum, vk::BufferUsageFlagBits::eStorageBuffer);

    for (uint32_t i = 0; i < coreflightFramesNum; i++) {
        std::copy(indirectDraws.begin(), indirectDraws.end(), static_cast<vk::DrawIndexedIndirectCommand *>(drawIndirectBuffer->get()) + indirectDraws.size() * i);
        std::copy(meshes.begin(), meshes.end(), static_cast<MeshData *>(meshesBuffer->get()) + meshes.size() * i);
        std::copy(objects.begin(), objects.end(), static_cast<ObjectData *>(objectsBuffer->get()) + objects.size() * i);
        std::copy(joints.begin(), joints.end(), static_cast<glm::mat4 *>(jointsBuffer->get()) + joints.size() * i);
    }

    drawIndirectBuffer.value().flush<1>(device, {{{0, sizeof(vk::DrawIndexedIndirectCommand) * indirectDraws.size() * coreflightFramesNum}}});
    meshesBuffer.value().flush<1>(device, {{{0, sizeof(MeshData) * meshes.size() * coreflightFramesNum}}});
    objectsBuffer.value().flush<1>(device, {{{0, sizeof(ObjectData) * objects.size() * coreflightFramesNum}}});
    jointsBuffer.value().flush<1>(device, {{{0, sizeof(glm::mat4) * joints.size() * coreflightFramesNum}}});
}

VulkanManagerCore::~VulkanManagerCore() {
    graphicsQueue.waitIdle();
}

void VulkanManagerCore::recreateRenderTarget(std::vector<RenderTargetHint> hints) {
    rprtd.clear();
    renderTargets.clear();
    std::transform(hints.begin(), hints.end(), std::back_inserter(renderTargets),
                   [this](const RenderTargetHint &hint) {
                       return createRenderTargetFromHint(device, hint);
                   });
    std::transform(renderTargets.begin(), renderTargets.end(), std::back_inserter(rprtd),
                   [this](const RenderTarget &rt) {
                       return defaultRenderProc->prepareRenderTargetDependant(rt);
                   });

    uniformBuffer.reset();
    uniformBuffer.emplace(physicalDevice, device, sizeof(SceneData) * renderTargets.size() * coreflightFramesNum, vk::BufferUsageFlagBits::eUniformBuffer);
    SceneData *dat = static_cast<SceneData *>(uniformBuffer->get());

    const auto &textures = modelManager.getTextureImageViews();

    {
        vk::DescriptorBufferInfo descUniformBufInfo[1];
        descUniformBufInfo[0].buffer = uniformBuffer->getBuffer();
        descUniformBufInfo[0].offset = 0;
        descUniformBufInfo[0].range = sizeof(SceneData);

        vk::DescriptorBufferInfo descObjectBufInfo[1];
        descObjectBufInfo[0].buffer = objectsBuffer->getBuffer();
        descObjectBufInfo[0].offset = 0;
        descObjectBufInfo[0].range = sizeof(ObjectData) * objects.size();

        vk::DescriptorBufferInfo descJointBufInfo[1];
        descJointBufInfo[0].buffer = jointsBuffer->getBuffer();
        descJointBufInfo[0].offset = 0;
        descJointBufInfo[0].range = sizeof(glm::mat4) * joints.size();

        vk::DescriptorBufferInfo descMeshBufInfo[1];
        descMeshBufInfo[0].buffer = meshesBuffer->getBuffer();
        descMeshBufInfo[0].offset = 0;
        descMeshBufInfo[0].range = sizeof(MeshData) * meshes.size();

        vk::WriteDescriptorSet writeDescSet[4];
        writeDescSet[0].dstSet = descSet.get();
        writeDescSet[0].dstBinding = 0;
        writeDescSet[0].dstArrayElement = 0;
        writeDescSet[0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
        writeDescSet[0].descriptorCount = std::size(descUniformBufInfo);
        writeDescSet[0].pBufferInfo = descUniformBufInfo;
        writeDescSet[1].dstSet = descSet.get();
        writeDescSet[1].dstBinding = 2;
        writeDescSet[1].dstArrayElement = 0;
        writeDescSet[1].descriptorType = vk::DescriptorType::eStorageBufferDynamic;
        writeDescSet[1].descriptorCount = std::size(descObjectBufInfo);
        writeDescSet[1].pBufferInfo = descObjectBufInfo;
        writeDescSet[2].dstSet = descSet.get();
        writeDescSet[2].dstBinding = 3;
        writeDescSet[2].dstArrayElement = 0;
        writeDescSet[2].descriptorType = vk::DescriptorType::eStorageBufferDynamic;
        writeDescSet[2].descriptorCount = std::size(descJointBufInfo);
        writeDescSet[2].pBufferInfo = descJointBufInfo;
        writeDescSet[3].dstSet = descSet.get();
        writeDescSet[3].dstBinding = 4;
        writeDescSet[3].dstArrayElement = 0;
        writeDescSet[3].descriptorType = vk::DescriptorType::eStorageBufferDynamic;
        writeDescSet[3].descriptorCount = std::size(descMeshBufInfo);
        writeDescSet[3].pBufferInfo = descMeshBufInfo;

        device.updateDescriptorSets(writeDescSet, {});
    }

    for (uint32_t j = 0; j < renderTargets.size(); j++) {
        for (uint32_t i = 0; i < coreflightFramesNum; i++) {
            dat[j * coreflightFramesNum + i].view = glm::lookAt(glm::vec3(0.0f, 1.3f, -0.9f), glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            dat[j * coreflightFramesNum + i].proj = glm::perspective(glm::radians(45.0f), float(renderTargets[0].extent.width) / float(renderTargets[0].extent.height), 0.1f, 10.0f);
        }
    }
}

vk::Fence VulkanManagerCore::render(uint32_t imageIndex,
                                    std::initializer_list<vk::Semaphore> waitSemaphores,
                                    std::initializer_list<vk::PipelineStageFlags> waitStages,
                                    std::initializer_list<vk::Semaphore> signalSemaphores) {
    auto currentFence = renderCmdBufFences[flightIndex].get();
    auto currentCmdBuf = renderCmdBufs[flightIndex].get();
    auto currentDescSet = descSet.get();

    device.waitForFences({currentFence}, true, UINT64_MAX);
    device.resetFences({currentFence});

    {
        CommandRec cmd{currentCmdBuf};

        for (uint32_t targetIndex = 0; targetIndex < renderTargets.size(); targetIndex++) {
            RenderDetails rd;
            rd.cmdBuf = currentCmdBuf;
            modelManager.prepareRender(rd);
            rd.descSet = currentDescSet;
            rd.dynamicOfs = {
                uint32_t(sizeof(SceneData) * (targetIndex * coreflightFramesNum + flightIndex)),
                uint32_t(sizeof(ObjectData) * objects.size() * flightIndex),
                uint32_t(sizeof(glm::mat4) * joints.size() * flightIndex),
                uint32_t(sizeof(MeshData) * meshes.size() * flightIndex),
            };
            rd.imageIndex = imageIndex;

            rd.modelsCount = indirectDraws.size();
            rd.drawBuf = drawIndirectBuffer.value().getBuffer();
            rd.drawBufOffset = 0;
            rd.drawBufStride = sizeof(vk::DrawIndexedIndirectCommand);

            defaultRenderProc->render(rd, renderTargets[targetIndex], rprtd[targetIndex]);
        }
    }

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

    flightIndex = (flightIndex + 1) % coreflightFramesNum;

    return currentFence;
}
