#include "SimpleRenderProc.hpp"

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

vk::UniquePipeline SimpleRenderProc::createPipeline(vk::Device device, vk::Extent2D extent, vk::RenderPass renderpass, vk::PipelineLayout pipelineLayout) {
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
    vertBindings[0].stride = sizeof(SimpleVertex);
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

SimpleRenderProc::SimpleRenderProc(vk::Device _device, vk::DescriptorSetLayout descLayout)
    : device(_device) {
    pipelinelayout = createPipelineLayout(device, {descLayout});

    auto featVertShader = std::async(std::launch::async, [this]() { return createShaderModuleFromFile(device, "shader.vert.spv"); });
    auto featFragShader = std::async(std::launch::async, [this]() { return createShaderModuleFromFile(device, "shader.frag.spv"); });
    shaders.push_back(featVertShader.get());
    shaders.push_back(featFragShader.get());
}

RenderProcRenderTargetDependant SimpleRenderProc::prepareRenderTargetDependant(const RenderTarget &rt) {
    RenderProcRenderTargetDependant d;
    d.renderpass = createRenderPass(device, rt.format);
    d.pipeline = createPipeline(device, rt.extent, d.renderpass.get(), pipelinelayout.get());
    d.frameBufs = createFrameBufsFromImageView(device, d.renderpass.get(), rt.extent, {rt.imageViews});
    return d;
}

void SimpleRenderProc::render(const RenderDetails &rd, const RenderTarget &rt, const RenderProcRenderTargetDependant &rprtd) {
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

    cmdBuf.drawIndexedIndirect(rd.drawBuf, 0, rd.modelsCount, sizeof(vk::DrawIndexedIndirectCommand));
    cmdBuf.endRenderPass();
}

SimpleRenderProc::~SimpleRenderProc() {}
