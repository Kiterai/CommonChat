#include "Image.hpp"
#include "Buffer.hpp"
#include "Helper.hpp"

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
