#include "Helper.hpp"
#include <vector>
#include <vulkan/vulkan.hpp>

constexpr uint32_t bufNum = 8;

class CommandExecuter {
    vk::UniqueCommandPool pool;
    std::vector<vk::UniqueCommandBuffer> bufs;

  public:
    CommandExecuter(vk::Device device, uint32_t queueFamilyIndex)
        : pool{createCommandPool(device, queueFamilyIndex)},
          bufs(createCommandBuffers(device, pool.get(), bufNum)) {}
};
