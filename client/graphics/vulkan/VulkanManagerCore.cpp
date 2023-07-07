#include "VulkanManagerCore.hpp"
#include "Helper.hpp"
#include <future>
using namespace std::string_literals;

vk::UniqueRenderPass createRenderPassFromSwapchain(vk::Device device, const SwapchainDetails &swapchain) {
    vk::AttachmentDescription attachments[1];
    attachments[0].format = swapchain.format;
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

vk::UniquePipelineLayout createPipelineLayout(vk::Device device) {
    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = nullptr;

    return device.createPipelineLayoutUnique(layoutCreateInfo);
}

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

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

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

    auto featVertShader = std::async(std::launch::async, [device]() { return createShaderModuleFromFile(device, "shader.vert.spv"); });
    auto featFragShader = std::async(std::launch::async, [device]() { return createShaderModuleFromFile(device, "shader.frag.spv"); });

    vk::UniqueShaderModule vertShader = featVertShader.get();
    vk::UniqueShaderModule fragShader = featFragShader.get();

    vk::PipelineShaderStageCreateInfo shaderStage[2];
    shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStage[0].module = vertShader.get();
    shaderStage[0].pName = "main";
    shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStage[1].module = fragShader.get();
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

RenderTarget createRenderTargetFromHint(vk::Device device, const RenderTargetHint hint) {
    RenderTarget rt;

    rt.imageViews = createImageViewsFromImages(device, hint.images, hint.format);
    rt.frameBufs = createFrameBufsFromImageView(device, {}, hint.extent, {std::ref(rt.imageViews)});

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
      cmdPool{createCommandPool(device, queueSet.graphicsQueueFamilyIndex)} {
}

void VulkanManagerCore::recreateRenderTarget(std::vector<RenderTargetHint> hints) {
    renderTargets.clear();
    std::transform(hints.begin(), hints.end(), std::back_inserter(renderTargets),
                   [this](RenderTargetHint hint) {
                       return createRenderTargetFromHint(device, hint);
                   });
}
