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

vk::UniqueImageView createImageViewFromImage(vk::Device device, const vk::Image &image, vk::Format format, uint32_t arrayNum, vk::ImageAspectFlags aspect) {
    vk::ImageViewCreateInfo imgViewCreateInfo;
    imgViewCreateInfo.image = image;
    imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imgViewCreateInfo.format = format;
    imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    imgViewCreateInfo.subresourceRange.aspectMask = aspect;
    imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imgViewCreateInfo.subresourceRange.levelCount = 1;
    imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imgViewCreateInfo.subresourceRange.layerCount = arrayNum;

    return device.createImageViewUnique(imgViewCreateInfo);
}

std::vector<vk::UniqueImageView> createImageViewsFromImages(vk::Device device, const std::vector<vk::Image> &images, vk::Format format, vk::ImageAspectFlags aspect) {
    std::vector<vk::UniqueImageView> imageViews(images.size());

    for (uint32_t i = 0; i < images.size(); i++) {
        imageViews[i] = createImageViewFromImage(device, images[i], format, 1, aspect);
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

std::optional<uint32_t> findMemoryTypeIndex(vk::PhysicalDevice physDevice, std::optional<vk::MemoryPropertyFlags> memFlagReq, std::optional<vk::MemoryRequirements> memReq) {
    std::optional<uint32_t> index = std::nullopt;
    const vk::PhysicalDeviceMemoryProperties memoryProps = physDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++) {
        bool memReqOk = !memReq.has_value() || ((memReq->memoryTypeBits >> i) & 1);
        bool memFlagOk = !memFlagReq.has_value() || ((memoryProps.memoryTypes[i].propertyFlags & *memFlagReq) == memFlagReq);
        if (memReqOk && memFlagOk) {
            index = i;
            break;
        }
    }
    return index;
}

void writeByMemoryMapping(vk::Device device, vk::DeviceMemory memory, const void *src, size_t sz, vk::DeviceSize dstOffset) {
    auto pMem = device.mapMemory(memory, dstOffset, sz);
    std::memcpy(pMem, src, sz);
    device.flushMappedMemoryRanges({vk::MappedMemoryRange{memory, dstOffset, sz}});
    device.unmapMemory(memory);
}

void writeByBufferCopy(vk::Device device, vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Buffer srcBuf, vk::Buffer dstBuf, vk::DeviceSize sz, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset, vk::Fence fence) {
    CommandExec cmd{cmdBuf, queue, fence};

    vk::BufferCopy bufCopy;
    bufCopy.size = sz;
    bufCopy.srcOffset = srcOffset;
    bufCopy.dstOffset = dstOffset;
    cmdBuf.copyBuffer(srcBuf, dstBuf, {bufCopy});
}

void writeByBufferToImageCopy(vk::Device device, vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Buffer srcBuf, vk::Image dstImg, vk::Extent3D extent, uint32_t arrayNum, vk::DeviceSize srcOffset, vk::Fence fence) {
    CommandExec cmd{cmdBuf, queue, fence};

    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eUndefined;
        barrior.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = dstImg;
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = arrayNum;
        barrior.srcAccessMask = {};
        barrior.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                               vk::DependencyFlags{}, {}, {}, {barrior});
    }

    vk::BufferImageCopy bufimgCopy;
    bufimgCopy.bufferOffset = 0;
    bufimgCopy.bufferRowLength = 0;
    bufimgCopy.bufferImageHeight = 0;

    bufimgCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    bufimgCopy.imageSubresource.mipLevel = 0;
    bufimgCopy.imageSubresource.baseArrayLayer = 0;
    bufimgCopy.imageSubresource.layerCount = arrayNum;

    bufimgCopy.imageOffset = vk::Offset3D{0, 0, 0};
    bufimgCopy.imageExtent = extent;

    cmdBuf.copyBufferToImage(srcBuf, dstImg, vk::ImageLayout::eTransferDstOptimal, {bufimgCopy});

    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = dstImg;
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = arrayNum;
        barrior.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrior.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                               vk::DependencyFlags{}, {}, {}, {barrior});
    }
}
