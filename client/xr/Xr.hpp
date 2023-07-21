#include "../graphics/vulkan/VulkanOpenxrAdapter.hpp"
#include <memory>
#include <openxr/openxr.hpp>
#include <openxr/openxr_platform.h>
#include <vulkan/vulkan.hpp>

struct OpenxrSwapchainDetails {
    xr::UniqueSwapchain swapchain;
    xr::Extent2Di extent;
    int64_t format;
};

class XrManager {
  private:
    xr::UniqueInstance instance;
    xr::SystemId systemId;
    std::unique_ptr<VulkanManagerOpenxr> graphicsManager;
    xr::UniqueSession session;
    std::vector<OpenxrSwapchainDetails> swapchains;

    xr::EventDataBuffer evBuf;
    bool session_running = false, shouldExit = false;

    void HandleSessionStateChange(const xr::EventDataSessionStateChanged &ev);
    bool PollOneEvent();
    void PollEvent();
    void RenderFrame();

  public:
    XrManager();
    ~XrManager();

    void mainLoop();
};
