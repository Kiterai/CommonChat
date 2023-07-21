#include "Image.hpp"
#include "Buffer.hpp"
#include "Helper.hpp"

namespace {

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

void writeByMemoryMapping(vk::Device device, vk::DeviceMemory memory, void *src, size_t sz, vk::DeviceSize dstOffset) {
    auto pMem = device.mapMemory(memory, dstOffset, sz);
    std::memcpy(pMem, src, sz);
    device.flushMappedMemoryRanges({vk::MappedMemoryRange{memory, dstOffset, sz}});
    device.unmapMemory(memory);
}

void writeByBufferToImageCopy(vk::Device device, vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Buffer srcBuf, vk::Image dstImg, vk::Extent3D extent, uint32_t arrayNum, vk::DeviceSize srcOffset, vk::Fence fence) {
    CommandExec cmd{cmdBuf, queue, fence};

    {
        vk::ImageMemoryBarrier barrior;
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
        barrior.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrior.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                               vk::DependencyFlags{}, {}, {}, {barrior});
    }
}

} // namespace

Image::Image(vk::PhysicalDevice physDevice, vk::Device device, vk::Extent3D extent, uint32_t arrayNum, vk::ImageUsageFlags usage, std::optional<vk::MemoryPropertyFlags> memFlagReq) {
    vk::ImageCreateInfo imgCreateInfo;
    imgCreateInfo.imageType = vk::ImageType::e2D;
    imgCreateInfo.extent = extent;
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = arrayNum;
    imgCreateInfo.format = vk::Format::eR8G8B8A8Srgb;
    imgCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imgCreateInfo.usage = usage;
    imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imgCreateInfo.samples = vk::SampleCountFlagBits::e1;
    image = device.createImageUnique(imgCreateInfo);

    auto memReq = device.getImageMemoryRequirements(image.get());
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(physDevice, memFlagReq, memReq).value();
    memory = device.allocateMemoryUnique(allocInfo);

    device.bindImageMemory(image.get(), memory.get(), 0);
}

ReadonlyImage::ReadonlyImage(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::Extent3D extent, uint32_t arrayNum, vk::ImageUsageFlags usage, vk::Fence fence)
    : Image{physDevice, device, extent, arrayNum, usage | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal} {
    write(physDevice, device, queue, cmdBuf, datSrc, extent, arrayNum, 0, fence);
}

ReadonlyImage::ReadonlyImage(vk::PhysicalDevice physDevice, vk::Device device, vk::Extent3D extent, uint32_t arrayNum, vk::ImageUsageFlags usage)
    : Image{physDevice, device, extent, arrayNum, usage | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal} {
}

void ReadonlyImage::write(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::Extent3D extent, uint32_t arrayNum, vk::DeviceSize offset, vk::Fence fence) {
    auto sz = extent.width * extent.height * extent.depth * arrayNum * 4;
    Buffer stagingBuf{physDevice, device, sz, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible};
    device.waitForFences({fence}, true, UINT64_MAX);
    device.resetFences({fence});
    writeByMemoryMapping(device, stagingBuf.getMemory(), datSrc, sz, 0);
    writeByBufferToImageCopy(device, cmdBuf, queue, stagingBuf.getBuffer(), image.get(), extent, arrayNum, offset, fence);
    device.waitForFences({fence}, true, UINT64_MAX);
}
