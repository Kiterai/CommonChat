#include "../IGraphics.hpp"
#include "VulkanManagerCore.hpp"
#include <vulkan/vulkan.hpp>
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>
#include <optional>

struct OpenxrRenderTargetHint {
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
    std::unique_ptr<xr::impl::InputStructBase> getXrGraphicsBinding();
    std::optional<int64_t> chooseXrSwapchainFormat(const std::vector<int64_t> formats);

    VulkanManagerOpenxr(xr::Instance xrInst, xr::SystemId xrSysId);

    void buildRenderTarget(std::vector<OpenxrRenderTargetHint> swapchains);

    void render();
};
