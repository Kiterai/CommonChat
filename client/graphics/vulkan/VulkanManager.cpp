#include "VulkanManager.hpp"
#include "../../desktop/GLFWHelper.hpp"
#include <algorithm>
#include <optional>
#ifdef _DEBUG
#include <iostream>
#include <vulkan/vulkan_enums.hpp>
#endif
using namespace std::string_view_literals;

vk::UniqueInstance createVulkanInstanceWithGlfw() {
    std::vector<const char *> exts, layers;

    {
        uint32_t glfwExtCnt;
        auto glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCnt);
        if (!glfwExts)
            __GLFW_TERMINATE_ERROR_THROW
        for (uint32_t i = 0; i < glfwExtCnt; i++) {
            exts.push_back(glfwExts[i]);
        }
    }
    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.enabledExtensionCount = exts.size();
    instCreateInfo.ppEnabledExtensionNames = exts.data();
    instCreateInfo.enabledLayerCount = layers.size();
    instCreateInfo.ppEnabledLayerNames = layers.data();
    return vk::createInstanceUnique(instCreateInfo);
}

vk::UniqueSurfaceKHR createVulkanSurfaceWithGlfw(const vk::Instance instance, GLFWwindow *window) {
    VkSurfaceKHR c_surface;
    glfwCreateWindowSurface(instance, window, nullptr, &c_surface);
    return vk::UniqueSurfaceKHR{c_surface, instance};
}

struct UsingQueueSet {
    uint32_t graphicsQueueFamilyIndex;
};

struct Swapchain {
    vk::Format format;
    vk::Extent2D extent;
    vk::UniqueSwapchainKHR swapchain;
};

struct RenderTarget {
    Swapchain swapchain;
    std::vector<vk::UniqueImageView> imageViews;
    std::vector<vk::UniqueFramebuffer> frameBufs;
};

std::optional<UsingQueueSet> chooseSuitableQueueSet(const std::vector<vk::QueueFamilyProperties> queueProps) {
    UsingQueueSet props;
    bool existsGraphicsQueue = false;

    for (uint32_t j = 0; j < queueProps.size(); j++) {
        if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics) {
            existsGraphicsQueue = true;
            props.graphicsQueueFamilyIndex = j;
            break;
        }
    }

    if (!existsGraphicsQueue)
        return std::nullopt;
    return props;
}

vk::UniqueDevice createVulkanDeviceWithGlfw(vk::PhysicalDevice physicalDevice) {
    std::vector<const char *> exts;
    std::vector<const char *> layers;
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;

    exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    auto queueSet = chooseSuitableQueueSet(physicalDevice.getQueueFamilyProperties());
    if (!queueSet)
        throw std::exception("failed to setup vulkan queues");

    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.queueFamilyIndex = queueSet->graphicsQueueFamilyIndex;
    queueInfo.queueCount = 1;
    float queuePriorities[] = {1.0};
    queueInfo.pQueuePriorities = queuePriorities;
    queueInfos.push_back(queueInfo);

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.enabledExtensionCount = exts.size();
    deviceCreateInfo.ppEnabledExtensionNames = exts.data();
    deviceCreateInfo.enabledLayerCount = layers.size();
    deviceCreateInfo.ppEnabledLayerNames = layers.data();
    deviceCreateInfo.queueCreateInfoCount = queueInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueInfos.data();

    return physicalDevice.createDeviceUnique(deviceCreateInfo);
}

vk::PhysicalDevice chooseSuitablePhysicalDeviceWithGlfw(vk::Instance instance, vk::SurfaceKHR surface) {
    auto physicalDevices = instance.enumeratePhysicalDevices();

    std::optional<vk::PhysicalDevice> suitablePhysicalDevice;

    for (const auto &physicalDevice : physicalDevices) {
        auto queueProps = chooseSuitableQueueSet(physicalDevice.getQueueFamilyProperties());
        auto devExts = physicalDevice.enumerateDeviceExtensionProperties();

        auto swapchainExtSupport = std::find_if(devExts.begin(), devExts.end(), [](vk::ExtensionProperties extProp) {
                                       return std::string_view(extProp.extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                                   }) != devExts.end();

        bool supportsSurface =
            !physicalDevice.getSurfaceFormatsKHR(surface).empty() ||
            !physicalDevice.getSurfacePresentModesKHR(surface).empty();

        if (queueProps.has_value() && swapchainExtSupport && supportsSurface) {
            suitablePhysicalDevice = physicalDevice;
            break;
        }
    }

    if (!suitablePhysicalDevice)
        throw std::exception("suitable vulkan device not found");
    return suitablePhysicalDevice.value();
}

Swapchain createVulkanSwapchainWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface) {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

#ifdef _DEBUG
    std::clog << "surface formats:" << std::endl;
    for (const auto &surfaceFormat : surfaceFormats) {
        std::clog << "  " << vk::to_string(surfaceFormat.format) << " " << vk::to_string(surfaceFormat.colorSpace) << std::endl;
    }
    std::clog << "surface present modes:" << std::endl;
    for (const auto &surfacePresentMode : surfacePresentModes) {
        std::clog << "  " << vk::to_string(surfacePresentMode) << std::endl;
    }
#endif

    Swapchain swapchain;

    vk::SurfaceFormatKHR swapchainFormat = surfaceFormats[0];         // TODO
    vk::PresentModeKHR swapchainPresentMode = surfacePresentModes[0]; // TODO

    swapchain.format = swapchain.format;
    swapchain.extent = surfaceCapabilities.currentExtent;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    swapchainCreateInfo.imageFormat = swapchainFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchainFormat.colorSpace;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.clipped = VK_TRUE;

    swapchain.swapchain = device.createSwapchainKHRUnique(swapchainCreateInfo);

    return swapchain;
}

std::vector<vk::UniqueImageView> createImageViewsFromSwapchain(vk::Device device, const Swapchain &swapchain) {
    auto images = device.getSwapchainImagesKHR(swapchain.swapchain.get());
    std::vector<vk::UniqueImageView> imageViews(images.size());

    for (uint32_t i = 0; i < images.size(); i++) {
        const auto &image = images[i];

        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = image;
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imgViewCreateInfo.format = swapchain.format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        imageViews[i] = device.createImageViewUnique(imgViewCreateInfo);
    }

    return imageViews;
}

std::vector<vk::UniqueFramebuffer> createFrameBufsFromImageView(vk::Device device, vk::RenderPass renderpass, vk::Extent2D extent, const std::vector<std::reference_wrapper<const std::vector<vk::UniqueImageView>>> imageViews) {
    std::vector<vk::UniqueFramebuffer> frameBufs(imageViews.size());

    for (uint32_t i = 0; i < imageViews.size(); i++) {
        std::vector<vk::ImageView> frameBufAttachments(imageViews.size());
        for (uint32_t j = 0; j < imageViews.size(); j++) {
            frameBufAttachments[j] = imageViews[j].get()[i].get();
        }

        vk::FramebufferCreateInfo frameBufCreateInfo;
        frameBufCreateInfo.width = extent.width;
        frameBufCreateInfo.height = extent.height;
        frameBufCreateInfo.layers = 1;
        frameBufCreateInfo.renderPass = renderpass;
        frameBufCreateInfo.attachmentCount = frameBufAttachments.size();
        frameBufCreateInfo.pAttachments = frameBufAttachments.data();

        frameBufs[i] = device.createFramebufferUnique(frameBufCreateInfo);
    }

    return frameBufs;
}

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

std::vector<RenderTarget> createRenderTargetsWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface) {
    auto swapchain = createVulkanSwapchainWithGlfw(physicalDevice, device, surface);
    auto imgViews = createImageViewsFromSwapchain(device, swapchain);

    std::vector<RenderTarget> v;
    v.emplace_back(RenderTarget{std::move(swapchain), std::move(imgViews)});
    return v;
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
