#include "Buffer.hpp"
#include "Helper.hpp"

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
}

CommunicationBuffer::CommunicationBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize sz, vk::BufferUsageFlags usage)
    : Buffer{physDevice, device, sz, usage, vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible},
      device{device} {
    pMem = device.mapMemory(memory.get(), 0, sz);
}

CommunicationBuffer::CommunicationBuffer(CommunicationBuffer &&buf)
    : Buffer(std::move(buf)), device(buf.device), pMem(buf.pMem) {
    buf.pMem = nullptr;
}

CommunicationBuffer::~CommunicationBuffer() {
    if (pMem)
        device.unmapMemory(memory.get());
}
