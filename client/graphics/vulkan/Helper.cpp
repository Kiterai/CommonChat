#include "Helper.hpp"
#include <fstream>

std::optional<UsingQueueSet> chooseSuitableQueueSet(const std::vector<vk::QueueFamilyProperties> queueProps) {
    UsingQueueSet props;
    bool existsGraphicsQueue = false;

    for (uint32_t j = 0; j < queueProps.size(); j++) {
        if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics) {
            existsGraphicsQueue = true;
            props.graphicsQueueFamilyIndex = j;
            break;
        }
    }

    if (!existsGraphicsQueue)
        return std::nullopt;
    return props;
}

std::vector<vk::UniqueImageView> createImageViewsFromImages(vk::Device device, const std::vector<vk::Image> &images, vk::Format format) {
    std::vector<vk::UniqueImageView> imageViews(images.size());

    for (uint32_t i = 0; i < images.size(); i++) {
        const auto &image = images[i];

        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = image;
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imgViewCreateInfo.format = format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        imageViews[i] = device.createImageViewUnique(imgViewCreateInfo);
    }

    return imageViews;
}

std::vector<vk::UniqueFramebuffer> createFrameBufsFromImageView(vk::Device device, vk::RenderPass renderpass, vk::Extent2D extent, const std::vector<std::reference_wrapper<const std::vector<vk::UniqueImageView>>> imageViews) {
    const auto frameBufNum = imageViews[0].get().size();
    const auto frameBufImagesNum = imageViews.size();
#ifdef _DEBUG
    for (const auto &attachmentImageViews : imageViews)
        if (attachmentImageViews.get().size() != frameBufNum)
            throw std::runtime_error("number of imageview differs in createFrameBufsFromImageView()");
#endif

    std::vector<vk::UniqueFramebuffer> frameBufs(frameBufNum);

    for (uint32_t i = 0; i < frameBufNum; i++) {
        std::vector<vk::ImageView> frameBufAttachments(imageViews.size());
        for (uint32_t j = 0; j < frameBufImagesNum; j++) {
            frameBufAttachments[j] = imageViews[j].get()[i].get();
        }

        vk::FramebufferCreateInfo frameBufCreateInfo;
        frameBufCreateInfo.width = extent.width;
        frameBufCreateInfo.height = extent.height;
        frameBufCreateInfo.layers = 1;
        frameBufCreateInfo.renderPass = renderpass;
        frameBufCreateInfo.attachmentCount = frameBufAttachments.size();
        frameBufCreateInfo.pAttachments = frameBufAttachments.data();

        frameBufs[i] = device.createFramebufferUnique(frameBufCreateInfo);
    }

    return frameBufs;
}

std::vector<char> loadFile(const std::filesystem::path &path) {
    size_t fileSz = std::filesystem::file_size(path);

    std::ifstream file(path, std::ios_base::binary);

    std::vector<char> fileData(fileSz);
    file.read(fileData.data(), fileSz);

    return fileData;
}

vk::UniqueShaderModule createShaderModuleFromBinary(vk::Device device, const std::vector<char> &binary) {
    vk::ShaderModuleCreateInfo shaderCreateInfo;
    shaderCreateInfo.codeSize = binary.size();
    shaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(binary.data());

    return device.createShaderModuleUnique(shaderCreateInfo);
}

vk::UniqueShaderModule createShaderModuleFromFile(vk::Device device, const std::filesystem::path &path) {
    return createShaderModuleFromBinary(device, loadFile(path));
}

vk::UniqueCommandPool createCommandPool(vk::Device device, uint32_t queueFamilyIndex) {
    vk::CommandPoolCreateInfo poolCreateInfo;
    poolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

    return device.createCommandPoolUnique(poolCreateInfo);
}

std::vector<vk::UniqueCommandBuffer> createCommandBuffers(vk::Device device, vk::CommandPool pool, uint32_t n) {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = n;

    return device.allocateCommandBuffersUnique(allocInfo);
}

vk::UniqueCommandBuffer createCommandBuffer(vk::Device device, vk::CommandPool pool) {
    auto bufs = createCommandBuffers(device, pool, 1);

    return std::move(bufs[0]);
}

std::vector<vk::UniqueFence> createFences(vk::Device device, uint32_t n, bool signaled) {
    vk::FenceCreateInfo createInfo;
    if (signaled)
        createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    std::vector<vk::UniqueFence> fences;
    for (uint32_t i = 0; i < n; i++)
        fences.emplace_back(device.createFenceUnique(createInfo));
    return fences;
}

void Submit(std::initializer_list<vk::CommandBuffer> cmdBufs, vk::Queue queue, vk::Fence fence) {
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = cmdBufs.size();
    submitInfo.pCommandBuffers = cmdBufs.begin();
    queue.submit({submitInfo}, fence);
}
