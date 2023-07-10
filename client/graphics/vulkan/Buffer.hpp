#include <array>
#include <optional>
#include <vulkan/vulkan.hpp>

class Buffer {
  protected:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;

  public:
    Buffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize sz, vk::BufferUsageFlags usage, std::optional<vk::MemoryPropertyFlags> memFlagReq);

    vk::Buffer getBuffer() { return buffer.get(); };
    vk::DeviceMemory getMemory() { return memory.get(); };
};

class ReadonlyBuffer : public Buffer {
  public:
    ReadonlyBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::DeviceSize sz, vk::BufferUsageFlags usage, vk::Fence fence);
    ReadonlyBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::BufferUsageFlags usage, vk::DeviceSize sz);
    void write(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::DeviceSize sz, vk::DeviceSize dstOffset, vk::Fence fence);
};

class CommunicationBuffer : public Buffer {
    vk::Device device;
    void *pMem;

  public:
    CommunicationBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize sz, vk::BufferUsageFlags usage);
    ~CommunicationBuffer();
    void *get() const { return pMem; };
    template <size_t Count>
    void flush(vk::Device device, std::array<std::pair<vk::DeviceSize, vk::DeviceSize>, Count> ranges) {
        std::array<vk::MappedMemoryRange, Count> vkranges;
        for (uint32_t i = 0; i < ranges.size(); i++) {
            vkranges[i].memory = memory.get();
            vkranges[i].offset = ranges[i].first;
            vkranges[i].size = ranges[i].second;
        }
        device.flushMappedMemoryRanges(vkranges.size(), vkranges.data());
    }
};
