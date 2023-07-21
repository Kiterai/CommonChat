#ifndef VULKAN_IMAGE_HPP
#define VULKAN_IMAGE_HPP

#include <array>
#include <optional>
#include <vulkan/vulkan.hpp>

class Image {
  protected:
    vk::UniqueImage image;
    vk::UniqueDeviceMemory memory;

  public:
    Image(vk::PhysicalDevice physDevice, vk::Device device, vk::Extent3D extent, uint32_t arrayNum, vk::ImageUsageFlags usage, std::optional<vk::MemoryPropertyFlags> memFlagReq);
    Image(Image&&) = default;

    vk::Image getImage() const { return image.get(); };
    vk::DeviceMemory getMemory() const { return memory.get(); };
};

class ReadonlyImage : public Image {
  public:
    ReadonlyImage(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::Extent3D extent, uint32_t arrayNum, vk::ImageUsageFlags usage, vk::Fence fence);
    ReadonlyImage(vk::PhysicalDevice physDevice, vk::Device device, vk::Extent3D extent, uint32_t arrayNum, vk::ImageUsageFlags usage);
    ReadonlyImage(ReadonlyImage&&) = default;
    void write(vk::PhysicalDevice physDevice, vk::Device device, vk::Queue queue, vk::CommandBuffer cmdBuf, void *datSrc, vk::Extent3D extent, uint32_t arrayNum, vk::DeviceSize offset, vk::Fence fence);
};

#endif VULKAN_IMAGE_HPP
