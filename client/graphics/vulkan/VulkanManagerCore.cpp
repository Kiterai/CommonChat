#include "VulkanManagerCore.hpp"
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std::string_literals;

constexpr uint32_t renderCmdBufNum = 8;
constexpr uint32_t coreflightFramesNum = 2;

struct Vertex {
    float x, y, z;
};
std::vector<Vertex> vertices = {{0.5, 0.0, 0.0}, {-0.5, 0.0, 0.0}, {0.0, 0.0, 0.5}};

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

vk::UniqueRenderPass createRenderPass(vk::Device device, vk::Format renderTargetFormat) {
    vk::AttachmentDescription attachments[1];
    attachments[0].format = renderTargetFormat;
    attachments[0].samples = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference subpass0_attachmentRefs[1];
    subpass0_attachmentRefs[0].attachment = 0;
    subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpasses[1];
    subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpass0_attachmentRefs;

    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = attachments;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = subpasses;
    renderpassCreateInfo.dependencyCount = 0;
    renderpassCreateInfo.pDependencies = nullptr;

    return device.createRenderPassUnique(renderpassCreateInfo);
}

vk::UniquePipelineLayout createPipelineLayout(vk::Device device, std::initializer_list<vk::DescriptorSetLayout> descLayouts) {
    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setLayoutCount = descLayouts.size();
    layoutCreateInfo.pSetLayouts = descLayouts.begin();
    return device.createPipelineLayoutUnique(layoutCreateInfo);
}

RenderTarget createRenderTargetFromHint(vk::Device device, const RenderTargetHint hint, vk::PipelineLayout pipelinelayout) {
    RenderTarget rt;

    rt.extent = hint.extent;
    rt.format = hint.format;
    rt.imageViews = createImageViewsFromImages(device, hint.images, hint.format);
    // rt.renderpass = createRenderPass(device, hint.format);
    // rt.frameBufs = createFrameBufsFromImageView(device, rt.renderpass.get(), hint.extent, {std::ref(rt.imageViews)});
    // rt.pipeline = createPipeline(device, hint.extent, rt.renderpass.get(), pipelinelayout);

    return rt;
}

class SimpleRenderProc : public IRenderProc {
    vk::Device device;
    // vk::DescriptorPool pool;
    // vk::UniqueDescriptorSetLayout descLayout;
    // std::vector<vk::UniqueDescriptorSet> descSets;
    vk::UniquePipelineLayout pipelinelayout;
    std::vector<vk::UniqueShaderModule> shaders;

    vk::UniquePipeline createPipeline(vk::Device device, vk::Extent2D extent, vk::RenderPass renderpass, vk::PipelineLayout pipelineLayout) {
        vk::Viewport viewports[1];
        viewports[0].x = 0.0;
        viewports[0].y = 0.0;
        viewports[0].minDepth = 0.0;
        viewports[0].maxDepth = 1.0;
        viewports[0].width = extent.width;
        viewports[0].height = extent.height;

        vk::Rect2D scissors[1];
        scissors[0].offset = vk::Offset2D{0, 0};
        scissors[0].extent = extent;

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.pViewports = viewports;
        viewportState.scissorCount = 1;
        viewportState.pScissors = scissors;

        vk::VertexInputBindingDescription vertBindings[1];
        vk::VertexInputAttributeDescription vertAttrs[1];

        vertBindings[0].binding = 0;
        vertBindings[0].inputRate = vk::VertexInputRate::eVertex;
        vertBindings[0].stride = sizeof(Vertex);
        vertAttrs[0].binding = 0;
        vertAttrs[0].location = 0;
        vertAttrs[0].offset = 0;
        vertAttrs[0].format = vk::Format::eR32G32B32Sfloat;

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.vertexBindingDescriptionCount = std::size(vertBindings);
        vertexInputInfo.pVertexBindingDescriptions = vertBindings;
        vertexInputInfo.vertexAttributeDescriptionCount = std::size(vertAttrs);
        vertexInputInfo.pVertexAttributeDescriptions = vertAttrs;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = false;

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.depthClampEnable = false;
        rasterizer.rasterizerDiscardEnable = false;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = false;

        vk::PipelineMultisampleStateCreateInfo multisample;
        multisample.sampleShadingEnable = false;
        multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::PipelineColorBlendAttachmentState blendattachment[1];
        blendattachment[0].colorWriteMask =
            vk::ColorComponentFlagBits::eA |
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB;
        blendattachment[0].blendEnable = false;

        vk::PipelineColorBlendStateCreateInfo blend;
        blend.logicOpEnable = false;
        blend.attachmentCount = 1;
        blend.pAttachments = blendattachment;

        vk::PipelineShaderStageCreateInfo shaderStage[2];
        shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
        shaderStage[0].module = shaders[0].get();
        shaderStage[0].pName = "main";
        shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
        shaderStage[1].module = shaders[1].get();
        shaderStage[1].pName = "main";

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multisample;
        pipelineCreateInfo.pColorBlendState = &blend;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.renderPass = renderpass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStage;

        return device.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;
    }

  public:
    SimpleRenderProc(vk::Device _device, vk::DescriptorSetLayout descLayout)
        : device(_device) {
        // descLayout = createDescLayout(device);
        // descSets = createDescSets(device, pool, descLayout.get(), coreflightFramesNum);
        pipelinelayout = createPipelineLayout(device, {descLayout});

        auto featVertShader = std::async(std::launch::async, [this]() { return createShaderModuleFromFile(device, "shader.vert.spv"); });
        auto featFragShader = std::async(std::launch::async, [this]() { return createShaderModuleFromFile(device, "shader.frag.spv"); });
        shaders.push_back(featVertShader.get());
        shaders.push_back(featFragShader.get());
    }
    RenderProcRenderTargetDependant prepareRenderTargetDependant(const RenderTarget &rt) override {
        RenderProcRenderTargetDependant d;
        d.renderpass = createRenderPass(device, rt.format);
        d.pipeline = createPipeline(device, rt.extent, d.renderpass.get(), pipelinelayout.get());
        d.frameBufs = createFrameBufsFromImageView(device, d.renderpass.get(), rt.extent, {rt.imageViews});
        return d;
    };
    void render(const RenderDetails &rd, const RenderTarget &rt, const RenderProcRenderTargetDependant &rprtd) override {
        const auto cmdBuf = rd.cmdBuf;
        CommandRec cmd{cmdBuf};

        vk::ClearValue clearVal[1];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        vk::RenderPassBeginInfo rpBeginInfo;
        rpBeginInfo.renderPass = rprtd.renderpass.get();
        rpBeginInfo.framebuffer = rprtd.frameBufs[rd.imageIndex].get();
        rpBeginInfo.renderArea = vk::Rect2D{{0, 0}, rt.extent};
        rpBeginInfo.clearValueCount = 1;
        rpBeginInfo.pClearValues = clearVal;

        cmdBuf.beginRenderPass(rpBeginInfo, vk::SubpassContents::eInline);
        cmdBuf.bindVertexBuffers(0, {rd.vertBuf}, {0});
        cmdBuf.bindIndexBuffer(rd.indexBuf, 0, vk::IndexType::eUint32);
        cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout.get(), 0, {rd.descSet}, {});
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, rprtd.pipeline.get());

        cmdBuf.draw(3, 1, 0, 0);
        // cmdBuf.drawIndexedIndirect(rd.drawBuf, 0, rd.modelsCount, 20);
        cmdBuf.endRenderPass();
    };
    ~SimpleRenderProc() {}
};

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
                            static_cast<void *>(vertices.data()), vertices.size() * sizeof(Vertex),
                            vk::BufferUsageFlagBits::eVertexBuffer, assetManageFence.get());
    modelIndexBuffer.emplace(physicalDevice, device, graphicsQueue, assetManageCmdBuf.get(),
                             vertices.data(), vertices.size() * sizeof(Vertex),
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
