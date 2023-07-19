#include "./Xr.hpp"
#include <fmt/format.h>
#include <iostream>
#include <vector>

std::vector<const char *> requiredExtensions() {
    std::vector<const char *> exts;
    exts.push_back(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);

    return exts;
}

xr::UniqueInstance createInstance() {
    std::vector<const char *> layers = {};
    std::vector<const char *> exts = requiredExtensions();

    xr::InstanceCreateInfo instanceCreateInfo{};
    strcpy(instanceCreateInfo.applicationInfo.applicationName, "XRTest");
    strcpy(instanceCreateInfo.applicationInfo.engineName, "XRTest");
    instanceCreateInfo.applicationInfo.apiVersion = xr::Version::current();
    instanceCreateInfo.enabledApiLayerCount = layers.size();
    instanceCreateInfo.enabledApiLayerNames = layers.data();
    instanceCreateInfo.enabledExtensionCount = exts.size();
    instanceCreateInfo.enabledExtensionNames = exts.data();

    return xr::createInstanceUnique(instanceCreateInfo);
}

xr::SystemId getSystem(xr::Instance instance) {
    xr::SystemGetInfo sysGetInfo{};
    sysGetInfo.formFactor = xr::FormFactor::HeadMountedDisplay;

    return instance.getSystem(sysGetInfo);
}

xr::UniqueSession createSession(xr::Instance instance, xr::SystemId systemId, std::unique_ptr<xr::impl::InputStructBase> &&graphicsBinding) {
    xr::SessionCreateInfo sessionCreateInfo{};
    sessionCreateInfo.systemId = systemId;
    sessionCreateInfo.next = graphicsBinding.get();

    return instance.createSessionUnique(sessionCreateInfo);
}

std::vector<OpenxrSwapchainDetails> createSwapchain(xr::Instance instance, xr::SystemId systemId, xr::Session session, int64_t selectedSwapchainFmt) {
    auto configViews = instance.enumerateViewConfigurationViewsToVector(systemId, xr::ViewConfigurationType::PrimaryStereo);

    std::vector<OpenxrSwapchainDetails> swapchains;

    std::transform(configViews.begin(), configViews.end(), std::back_inserter(swapchains), [&](xr::ViewConfigurationView configView) {
        OpenxrSwapchainDetails swapchainDetails;

        xr::SwapchainCreateInfo createInfo;
        createInfo.arraySize = 1;
        createInfo.format = selectedSwapchainFmt;
        createInfo.width = configView.recommendedImageRectWidth;
        createInfo.height = configView.recommendedImageRectHeight;
        createInfo.mipCount = 1;
        createInfo.faceCount = 1;
        createInfo.sampleCount = configView.recommendedSwapchainSampleCount;
        createInfo.usageFlags = xr::SwapchainUsageFlagBits::ColorAttachment /* | xr::SwapchainUsageFlagBits::Sampled*/;

        swapchainDetails.swapchain = session.createSwapchainUnique(createInfo);
        swapchainDetails.format = selectedSwapchainFmt;
        swapchainDetails.extent.width = configView.recommendedImageRectWidth;
        swapchainDetails.extent.height = configView.recommendedImageRectHeight;

        return swapchainDetails;
    });

    return swapchains;
}

XrManager::XrManager()
    : instance{createInstance()},
      systemId{getSystem(instance.get())},
      graphicsManager{new VulkanManagerOpenxr(instance.get(), systemId)},
      session{createSession(instance.get(), systemId, graphicsManager->getXrGraphicsBinding())},
      swapchains(createSwapchain(instance.get(), systemId, session.get(), graphicsManager->chooseXrSwapchainFormat(session->enumerateSwapchainFormatsToVector()).value())) {
    {
        std::vector<OpenxrRenderTargetHint> hints;
        std::transform(swapchains.begin(), swapchains.end(), std::back_inserter(hints), [&](OpenxrSwapchainDetails &swapchain) {
            OpenxrRenderTargetHint hint;
            hint.swapchain = swapchain.swapchain.get();
            hint.extent = swapchain.extent;
            hint.format = swapchain.format;
            return hint;
        });
        graphicsManager->buildRenderTarget(hints);
    }

#ifdef _DEBUG
    auto extProps = xr::enumerateInstanceExtensionPropertiesToVector(nullptr);
    std::cout << "OpenXR Extensions: " << extProps.size() << std::endl;
    for (const auto &extProp : extProps) {
        std::cout << extProp.extensionName << " v" << extProp.extensionVersion << std::endl;
    }

    auto sysProp = instance->getSystemProperties(systemId);

    std::cout << fmt::format("SystemID: {:X}", this->systemId.get()) << std::endl;
    std::cout << fmt::format("SystemName: {}, vendorID: {:X}", sysProp.systemName, sysProp.vendorId) << std::endl;
    std::cout << fmt::format("Max Layers: {}, Max Size: {}x{}", sysProp.graphicsProperties.maxLayerCount, sysProp.graphicsProperties.maxSwapchainImageWidth, sysProp.graphicsProperties.maxSwapchainImageHeight) << std::endl;
    std::cout << fmt::format("Tracking: position/{}, orientation/{}", sysProp.trackingProperties.positionTracking.get(), sysProp.trackingProperties.orientationTracking.get()) << std::endl;
#endif
}

XrManager::~XrManager() {
}

void XrManager::mainLoop() {
    while (true) {
    }
}
