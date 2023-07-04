#include "VulkanManager.hpp"
#include "Helper.hpp"
#ifdef USE_DESKTOP_MODE
#include "SetupWithGlfw.hpp"
#endif
using namespace std::string_view_literals;

vk::UniqueRenderPass createRenderPassFromSwapchain(vk::Device device, const Swapchain &swapchain) {
    vk::AttachmentDescription attachments[1];
    attachments[0].format = swapchain.format;
    attachments[0].samples = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference subpass0_attachmentRefs[1];
    subpass0_attachmentRefs[0].attachment = 0;
    subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpasses[1];
    subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpass0_attachmentRefs;

    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = attachments;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = subpasses;
    renderpassCreateInfo.dependencyCount = 0;
    renderpassCreateInfo.pDependencies = nullptr;

    return device.createRenderPassUnique(renderpassCreateInfo);
}

class VulkanManager : public IGraphics {
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;
    Swapchain swapchain;
    std::vector<RenderTarget> renderTargets;

    VulkanManager() {}
    VulkanManager(GLFWwindow *window) : instance{createVulkanInstanceWithGlfw()},
                                        surface{createVulkanSurfaceWithGlfw(this->instance.get(), window)},
                                        physicalDevice{chooseSuitablePhysicalDeviceWithGlfw(this->instance.get(), this->surface.get())},
                                        device{createVulkanDeviceWithGlfw(this->physicalDevice)},
                                        renderTargets(createRenderTargetsWithGlfw(physicalDevice, device.get(), surface.get())) {
    }

  public:
    static pIGraphics makeFromDesktopGui(GLFWwindow *window) {
        return pIGraphics(new VulkanManager{window});
    }
    static pIGraphics makeFromXr(xr::Instance xrInst, xr::SystemId xrSysId) {
        auto pObj = new VulkanManager{};

        // TODO

        return pIGraphics(pObj);
    }
};

pIGraphics makeFromDesktopGui_Vulkan(GLFWwindow *window) {
    return VulkanManager::makeFromDesktopGui(window);
}

pIGraphics makeFromXr_Vulkan(xr::Instance xrInst, xr::SystemId xrSysId) {
    return VulkanManager::makeFromXr(xrInst, xrSysId);
}
