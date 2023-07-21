#include "./Xr.hpp"
#include <fmt/format.h>
#include <iostream>
#include <thread>
#include <vector>

#define XR_CHK_ERR(f)                       \
    if (auto result = f; XR_FAILED(result)) \
        throw std::runtime_error(fmt::format("Err: {}, {} {}", to_string(result), __LINE__, #f));

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

inline bool XrManager::PollOneEvent() {
    auto result = instance->pollEvent(evBuf);
    switch (result) {
    case xr::Result::Success:
        if (this->evBuf.type == xr::StructureType::EventDataEventsLost) {
            auto &eventsLost = *reinterpret_cast<xr::EventDataEventsLost *>(&this->evBuf);
            std::cout << "Event Lost: " << eventsLost.lostEventCount << std::endl;
        }
        return true;
    case xr::Result::EventUnavailable:
        return false;
    default:
        throw std::runtime_error(fmt::format("failed to poll event: {}", xr::to_string(result)));
    }
}

inline void XrManager::HandleSessionStateChange(const xr::EventDataSessionStateChanged &ev) {
    if (ev.session != this->session.get()) {
        std::cout << "Event from Unknown Session" << std::endl;
        return;
    }
    switch (ev.state) {
    case xr::SessionState::Ready: {
        xr::SessionBeginInfo beginInfo;
        beginInfo.primaryViewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
        session->beginSession(beginInfo);
        session_running = true;
        std::cout << "session began" << std::endl;
        break;
    }
    case xr::SessionState::Stopping: {
        session->endSession();
        session_running = false;
        std::cout << "session ended" << std::endl;
        break;
    }
    case xr::SessionState::Exiting:
    case xr::SessionState::LossPending:
        // session_running = false;
        shouldExit = true;
        break;
    default:
        break;
    }
}

inline void XrManager::PollEvent() {
    while (PollOneEvent()) {
        std::cout << fmt::format("Event: {}", to_string(this->evBuf.type)) << std::endl;
        switch (this->evBuf.type) {
        case xr::StructureType::EventDataSessionStateChanged: {
            const auto &ev = *reinterpret_cast<xr::EventDataSessionStateChanged *>(&this->evBuf);
            std::cout << "State -> " << to_string(ev.state) << std::endl;
            HandleSessionStateChange(ev);
        } break;
        default:
            break;
        }
    }
}

inline void XrManager::RenderFrame() {
    xr::FrameWaitInfo frameWaitInfo;
    auto frameState = session->waitFrame(frameWaitInfo);

    xr::FrameBeginInfo beginInfo{};
    XR_CHK_ERR(session->beginFrame(beginInfo));

    constexpr auto max_layers_num = 1;
    constexpr auto max_views_num = 2;

    std::array<xr::CompositionLayerBaseHeader *, max_layers_num> layers;
    std::array<xr::CompositionLayerProjectionView, max_views_num> projectionViews{};

    xr::FrameEndInfo endInfo;
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
    endInfo.layerCount = layers.size();
    endInfo.layers = layers.data();

    if (frameState.shouldRender) {
        for (uint32_t i = 0; i < swapchains.size(); i++) {
            const auto &swapchain = swapchains[i].swapchain.get();

            xr::SwapchainImageAcquireInfo acquireInfo;
            auto imageIndex = swapchain.acquireSwapchainImage(acquireInfo);

            xr::SwapchainImageWaitInfo waitInfo;
            waitInfo.timeout = xr::Duration::infinite();
            swapchain.waitSwapchainImage(waitInfo);

            projectionViews[i].type = xr::StructureType::CompositionLayerProjectionView;
            // projectionViews[i].pose = views[i].pose;   
            // projectionViews[i].fov = views[i].fov;
            projectionViews[i].subImage.swapchain = swapchain.get();
            projectionViews[i].subImage.imageRect.offset = xr::Offset2Di{0, 0};
            projectionViews[i].subImage.imageRect.extent = swapchains[i].extent;

            graphicsManager->render();

            xr::SwapchainImageReleaseInfo releaseInfo;
            swapchain.releaseSwapchainImage(releaseInfo);
        }
    }

    session->endFrame(endInfo);
}

void XrManager::mainLoop() {
    while (!shouldExit) {
        PollEvent();
        if (session_running) {
            // PollAction();
            RenderFrame();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
}
