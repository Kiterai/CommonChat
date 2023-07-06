#ifdef USE_DESKTOP_MODE

#include "SetupWithGlfw.hpp"
#include "../../desktop/GLFWHelper.hpp"
#include "Helper.hpp"

#ifdef _DEBUG
#include <iostream>
#include <vulkan/vulkan_enums.hpp>
#endif

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

vk::UniqueDevice createVulkanDeviceWithGlfw(vk::PhysicalDevice physicalDevice, const UsingQueueSet& queueSet) {
    std::vector<const char *> exts;
    std::vector<const char *> layers;
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;

    exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.queueFamilyIndex = queueSet.graphicsQueueFamilyIndex;
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

vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> formats) {
    for (const auto &format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return format;
    }
    return formats[0];
}

vk::PresentModeKHR chooseSurfacePresentMode(const std::vector<vk::PresentModeKHR> modes) {
    return vk::PresentModeKHR::eFifo;
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

    vk::SurfaceFormatKHR swapchainFormat = chooseSurfaceFormat(surfaceFormats);
    vk::PresentModeKHR swapchainPresentMode = chooseSurfacePresentMode(surfacePresentModes);

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

std::vector<RenderTarget> createRenderTargetsWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface) {
    auto swapchain = createVulkanSwapchainWithGlfw(physicalDevice, device, surface);
    auto imgViews = createImageViewsFromSwapchain(device, swapchain);

    std::vector<RenderTarget> v;
    v.emplace_back(RenderTarget{std::move(swapchain), std::move(imgViews)});
    return v;
}

#endif
