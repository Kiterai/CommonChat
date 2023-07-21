#ifdef USE_DESKTOP_MODE

#include "VulkanGlfwAdapter.hpp"
#include "../../desktop/GLFWHelper.hpp"

#ifdef _DEBUG
#include <iostream>
#include <vulkan/vulkan_enums.hpp>
#endif

vk::UniqueInstance createVulkanInstanceWithGlfw() {
    std::vector<const char *> exts, layers;

    const auto availableExts = vk::enumerateInstanceExtensionProperties();
    const auto availableLayers = vk::enumerateInstanceLayerProperties();

    auto checkExtAvailable = [&availableExts](const char *extName) {
        return std::find_if(availableExts.begin(), availableExts.end(),
                            [&](vk::ExtensionProperties prop) {
                                return std::string_view(prop.extensionName.data()) == extName;
                            }) != availableExts.end();
    };
    auto checkLayerAvailable = [&availableLayers](const char *layerName) {
        return std::find_if(availableLayers.begin(), availableLayers.end(),
                            [&](vk::LayerProperties prop) {
                                return std::string_view(prop.layerName.data()) == layerName;
                            }) != availableLayers.end();
    };

#ifdef _DEBUG
    std::clog << "Available Extensions:" << std::endl;
    for (const auto &ext : availableExts) {
        std::clog << "  " << ext.extensionName
                  << " (version: "
                  << ext.specVersion
                  << ")" << std::endl;
    }
    std::clog << "Available Layers:" << std::endl;
    for (const auto &layer : availableLayers) {
        std::clog << "  " << layer.layerName
                  << " (spec version: " << VK_VERSION_MAJOR(layer.specVersion) << "." << VK_VERSION_MINOR(layer.specVersion) << "." << VK_VERSION_PATCH(layer.specVersion)
                  << ", impl version: " << layer.implementationVersion
                  << ") : " << layer.description.data() << std::endl;
    }
    if (checkLayerAvailable("VK_LAYER_KHRONOS_validation")) {
        std::clog << "Validation Layer: on" << std::endl;
        layers.push_back("VK_LAYER_KHRONOS_validation");
    } else {
        std::clog << "Validation Layer: unavailable" << std::endl;
    }
#endif

    {
        uint32_t glfwExtCnt;
        auto glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCnt);
        if (!glfwExts)
            __GLFW_TERMINATE_ERROR_THROW
        for (uint32_t i = 0; i < glfwExtCnt; i++) {
            if (!checkExtAvailable(glfwExts[i]))
                throw std::runtime_error(std::string("Extension not supported: ") + glfwExts[i]);
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

vk::UniqueDevice createVulkanDeviceWithGlfw(vk::PhysicalDevice physicalDevice, const UsingQueueSet &queueSet) {
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
    
    auto devFeats = physicalDevice.getFeatures();

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &devFeats;
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
        auto devFeats = physicalDevice.getFeatures();

        auto swapchainExtSupport = std::find_if(devExts.begin(), devExts.end(), [](vk::ExtensionProperties extProp) {
                                       return std::string_view(extProp.extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                                   }) != devExts.end();

        bool supportsSurface =
            !physicalDevice.getSurfaceFormatsKHR(surface).empty() ||
            !physicalDevice.getSurfacePresentModesKHR(surface).empty();

        if (queueProps.has_value() && swapchainExtSupport && supportsSurface && devFeats.multiDrawIndirect) {
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

SwapchainDetails createVulkanSwapchainWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface) {
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

    SwapchainDetails swapchain;

    vk::SurfaceFormatKHR swapchainFormat = chooseSurfaceFormat(surfaceFormats);
    vk::PresentModeKHR swapchainPresentMode = chooseSurfacePresentMode(surfacePresentModes);

    swapchain.format = swapchainFormat.format;
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

std::vector<RenderTargetHint> getRenderTargetHintsWithGlfw(vk::PhysicalDevice physicalDevice, vk::Device device, const SwapchainDetails &swapchain) {
    std::vector<RenderTargetHint> v(1);
    v[0].format = swapchain.format;
    v[0].extent = swapchain.extent;
    v[0].images = std::move(device.getSwapchainImagesKHR(swapchain.swapchain.get()));
    return v;
}

void present(vk::Queue queue, vk::SwapchainKHR swapchain, uint32_t index, std::initializer_list<vk::Semaphore> waitSemaphores) {
    auto presentSwapchains = {swapchain};
    auto imgIndices = {index};

    vk::PresentInfoKHR presentInfo;
    presentInfo.swapchainCount = presentSwapchains.size();
    presentInfo.pSwapchains = presentSwapchains.begin();
    presentInfo.pImageIndices = imgIndices.begin();

    presentInfo.waitSemaphoreCount = waitSemaphores.size();
    presentInfo.pWaitSemaphores = waitSemaphores.begin();

    queue.presentKHR(presentInfo);
}

VulkanManagerGlfw::VulkanManagerGlfw(GLFWwindow *window) : instance{createVulkanInstanceWithGlfw()},
                                                           surface{createVulkanSurfaceWithGlfw(this->instance.get(), window)},
                                                           physicalDevice{chooseSuitablePhysicalDeviceWithGlfw(this->instance.get(), this->surface.get())},
                                                           queueSet{chooseSuitableQueueSet(physicalDevice.getQueueFamilyProperties()).value()},
                                                           device{createVulkanDeviceWithGlfw(this->physicalDevice, queueSet)},
                                                           presentQueue{this->device->getQueue(queueSet.graphicsQueueFamilyIndex, 0)},
                                                           core{instance.get(), physicalDevice, queueSet, device.get()} {}

VulkanManagerGlfw::~VulkanManagerGlfw() {
    presentQueue.waitIdle();
}

void VulkanManagerGlfw::buildRenderTarget() {
    swapchain = createVulkanSwapchainWithGlfw(physicalDevice, device.get(), surface.get());
    auto hints = getRenderTargetHintsWithGlfw(physicalDevice, device.get(), swapchain);
    core.recreateRenderTarget(hints);

    constexpr size_t flightFramesNumDefault = 2;
    flightFramesNum = std::min(flightFramesNumDefault, hints[0].images.size());

    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    imageAcquiredSemaphores.clear();
    imageRenderedSemaphores.clear();
    for (uint32_t i = 0; i < flightFramesNum; i++) {
        imageAcquiredSemaphores.emplace_back(device->createSemaphoreUnique(semaphoreCreateInfo));
        imageRenderedSemaphores.emplace_back(device->createSemaphoreUnique(semaphoreCreateInfo));
    }
    frameFlightFence.clear();
    frameFlightFence.resize(flightFramesNum);
}

void VulkanManagerGlfw::render() {
    if (frameFlightFence[flightFrameIndex])
        device->waitForFences({frameFlightFence[flightFrameIndex]}, true, UINT64_MAX);

    vk::ResultValue acquireImgResult =
        device->acquireNextImageKHR(swapchain.swapchain.get(), UINT64_MAX,
                                    imageAcquiredSemaphores[flightFrameIndex].get());
    if (acquireImgResult.result != vk::Result::eSuccess)
        throw std::runtime_error("failed to acquire image");

    frameFlightFence[flightFrameIndex] =
        core.render(acquireImgResult.value,
                    {imageAcquiredSemaphores[flightFrameIndex].get()},
                    {vk::PipelineStageFlagBits::eColorAttachmentOutput},
                    {imageRenderedSemaphores[flightFrameIndex].get()});

    present(presentQueue, swapchain.swapchain.get(), acquireImgResult.value,
            {imageRenderedSemaphores[flightFrameIndex].get()});

    flightFrameIndex++;
    if (flightFrameIndex >= flightFramesNum)
        flightFrameIndex = 0;
}

#endif
