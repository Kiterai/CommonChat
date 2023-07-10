#include "Buffer.hpp"
#include "Helper.hpp"

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

void writeByBufferCopy(vk::Device device, vk::CommandBuffer cmdBuf, vk::Queue queue, vk::Buffer srcBuf, vk::Buffer dstBuf, vk::DeviceSize sz, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset, vk::Fence fence) {
    CommandExec cmd{cmdBuf, queue, fence};

    vk::BufferCopy bufCopy;
    bufCopy.size = sz;
    bufCopy.srcOffset = srcOffset;
    bufCopy.dstOffset = dstOffset;
    cmdBuf.copyBuffer(srcBuf, dstBuf, {bufCopy});
}

Buffer::Buffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize sz, vk::BufferUsageFlags usage, std::optional<vk::MemoryPropertyFlags> memFlagReq) {
    vk::BufferCreateInfo bufCreateInfo;
    bufCreateInfo.size = sz;
    bufCreateInfo.usage = usage;
    bufCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    buffer = device.createBufferUnique(bufCreateInfo);

    auto memReq = device.getBufferMemoryRequirements(buffer.get());
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(physDevice, memFlagReq, memReq).value();
    memory = device.allocateMemoryUnique(allocInfo);

    device.bindBufferMemory(buffer.get(), memory.get(), 0);
}

ReadonlyBuffer::ReadonlyBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::DeviceSize sz, vk::BufferUsageFlags usage, vk::Fence fence)
    : Buffer{physDevice, device, sz, usage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal} {
    write(physDevice, device, queue, cmdBuf, datSrc, sz, 0, fence);
}

ReadonlyBuffer::ReadonlyBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::BufferUsageFlags usage, vk::DeviceSize sz)
    : Buffer{physDevice, device, sz, usage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal} {
}

void ReadonlyBuffer::write(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::DeviceSize sz, vk::DeviceSize offset, vk::Fence fence) {
    Buffer stagingBuf{physDevice, device, sz, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible};
    device.waitForFences({fence}, true, UINT64_MAX);
    device.resetFences({fence});
    writeByMemoryMapping(device, stagingBuf.getMemory(), datSrc, sz, 0);
    writeByBufferCopy(device, cmdBuf, queue, stagingBuf.getBuffer(), buffer.get(), sz, 0, offset, fence);
    device.waitForFences({fence}, true, UINT64_MAX);
    device.resetFences({fence});
}

CommunicationBuffer::CommunicationBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize sz, vk::BufferUsageFlags usage)
    : Buffer{physDevice, device, sz, usage, vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible},
      device{device} {
    pMem = device.mapMemory(memory.get(), 0, sz);
}

CommunicationBuffer::~CommunicationBuffer() {
    device.unmapMemory(memory.get());
}
