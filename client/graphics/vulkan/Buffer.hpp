#include <optional>
#include <vulkan/vulkan.hpp>

class Buffer {
  protected:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;

  public:
    Buffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize sz, vk::BufferUsageFlagBits usage, std::optional<vk::MemoryPropertyFlags> memFlagReq);

    vk::Buffer getBuffer();
    vk::DeviceMemory getMemory();
};

class ReadonlyBuffer : public Buffer {
  public:
    ReadonlyBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::DeviceSize sz, vk::BufferUsageFlagBits usage, vk::Fence fence);
};

class CommunicationBuffer : public Buffer {
    vk::Device device;
    void *pMem;

  public:
    CommunicationBuffer(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::DeviceSize sz, vk::BufferUsageFlagBits usage, vk::Fence fence);
    ~CommunicationBuffer();
    void *get() const;
};
