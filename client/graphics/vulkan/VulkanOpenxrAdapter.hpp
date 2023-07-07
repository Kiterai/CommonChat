#include "../IGraphics.hpp"
#include "Helper.hpp"
#include "VulkanManagerCore.hpp"
#include <vulkan/vulkan.hpp>
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>
#include <optional>

struct OpenxrSwapchainDetails {
    xr::Swapchain swapchain;
    int64_t format;
    xr::Extent2Di extent;
};

class VulkanManagerOpenxr : public IGraphics {
    vk::Instance vkInst;
    vk::PhysicalDevice vkPhysDevice;
    UsingQueueSet vkQueueSet;
    vk::Device vkDevice;

    VulkanManagerCore core;

  public:
    VulkanManagerOpenxr(xr::Instance xrInst, xr::SystemId xrSysId);

    void buildRenderTarget(std::vector<OpenxrSwapchainDetails> swapchains);
};
