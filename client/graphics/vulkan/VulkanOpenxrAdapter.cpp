#include "VulkanOpenxrAdapter.hpp"
using namespace std::literals::string_literals;

vk::Instance createVulkanInstanceWithOpenxr(xr::Instance xrInstance, xr::SystemId xrSystemId) {
    auto graphicsRequirements = xrInstance.getVulkanGraphicsRequirements2KHR(xrSystemId, xr::DispatchLoaderDynamic{xrInstance});

    std::vector<const char *> exts;
    std::vector<const char *> layers;

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "XRTest";
    appInfo.apiVersion = 1;
    appInfo.pEngineName = "";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = exts.size();
    createInfo.ppEnabledExtensionNames = exts.data();
    createInfo.enabledLayerCount = layers.size();
    createInfo.ppEnabledLayerNames = layers.data();

    const VkInstanceCreateInfo c_createInfo = (VkInstanceCreateInfo)createInfo;

    xr::VulkanInstanceCreateInfoKHR xrCreateInfo{};
    xrCreateInfo.systemId = xrSystemId;
    xrCreateInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    xrCreateInfo.vulkanCreateInfo = &c_createInfo;

    VkInstance tmpInst;
    VkResult vkResult;

    xrInstance.createVulkanInstanceKHR(xrCreateInfo, &tmpInst, &vkResult, xr::DispatchLoaderDynamic{xrInstance});
    if (vkResult != VK_SUCCESS)
        throw std::runtime_error("Vulkan Instance Creation Error: "s + to_string(vk::Result(vkResult)));
    return vk::Instance{tmpInst};
}

vk::PhysicalDevice getPhysicalDeviceWithOpenxr(xr::Instance xrInstance, xr::SystemId xrSystemId, vk::Instance vkInstance) {
    xr::VulkanGraphicsDeviceGetInfoKHR getInfo{};
    getInfo.systemId = xrSystemId;
    getInfo.vulkanInstance = vkInstance;

    return xrInstance.getVulkanGraphicsDevice2KHR(getInfo, xr::DispatchLoaderDynamic{xrInstance});
}

vk::Device createDeviceWithOpenxr(xr::Instance xrInstance, xr::SystemId xrSystemId, vk::PhysicalDevice physicalDevice) {
    std::vector<float> queuePriorities = {0.0f};

    auto usingQueueSet = chooseSuitableQueueSet(physicalDevice.getQueueFamilyProperties());

    vk::DeviceQueueCreateInfo graphicsQueueInfo;
    graphicsQueueInfo.queueCount = queuePriorities.size();
    graphicsQueueInfo.pQueuePriorities = queuePriorities.data();
    graphicsQueueInfo.queueFamilyIndex = usingQueueSet->graphicsQueueFamilyIndex;

    std::vector<vk::DeviceQueueCreateInfo> queueInfo = {graphicsQueueInfo};

    std::vector<const char *> exts;
    std::vector<const char *> layers;

    vk::PhysicalDeviceFeatures features{};

    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount = queueInfo.size();
    createInfo.pQueueCreateInfos = queueInfo.data();
    createInfo.enabledExtensionCount = exts.size();
    createInfo.ppEnabledExtensionNames = exts.data();
    createInfo.enabledLayerCount = layers.size();
    createInfo.ppEnabledLayerNames = layers.data();

    const VkDeviceCreateInfo c_createInfo = (VkDeviceCreateInfo)createInfo;

    xr::VulkanDeviceCreateInfoKHR xrCreateInfo{};
    xrCreateInfo.systemId = xrSystemId;
    xrCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    xrCreateInfo.vulkanCreateInfo = &c_createInfo;
    xrCreateInfo.vulkanPhysicalDevice = physicalDevice;

    VkDevice tmpDevice;
    VkResult vkResult;
    xrInstance.createVulkanDeviceKHR(xrCreateInfo, &tmpDevice, &vkResult, xr::DispatchLoaderDynamic{xrInstance});
    if (vkResult != VK_SUCCESS)
        throw std::runtime_error("Vulkan Device Creation Error: "s + to_string(vk::Result(vkResult)));
    return vk::Device{tmpDevice};
}

std::optional<int64_t> chooseXrSwapchainFormat(const std::vector<int64_t> formats) {
    std::vector<vk::Format> supportFormats = {
        vk::Format::eB8G8R8A8Srgb,
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eR8G8B8A8Unorm,
    };
    for (const auto supportFormat : supportFormats) {
        for (const auto usableFormat : formats) {
            if (vk::Format(usableFormat) == supportFormat) {
                return std::make_optional(usableFormat);
            }
        }
    }
    return std::nullopt;
}

std::vector<vk::Image> getImagesFromXrSwapchain(xr::Swapchain swapchain) {
    auto images = swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();

    std::vector<vk::Image> vkImages;
    std::transform(images.begin(), images.end(), std::back_inserter(vkImages),
                   [](xr::SwapchainImageVulkanKHR image) { return vk::Image(image.image); });

    return vkImages;
}

std::vector<RenderTargetHint> getRenderTargetHintsWithOpenxr(const std::vector<OpenxrSwapchainDetails> &swapchains) {
    std::vector<RenderTargetHint> v;
    std::transform(swapchains.begin(), swapchains.end(), std::back_inserter(v),
                   [](const OpenxrSwapchainDetails &swapchain) {
                       RenderTargetHint hint;
                       hint.format = vk::Format(swapchain.format);
                       hint.extent = vk::Extent2D(swapchain.extent.width, swapchain.extent.height);
                       hint.images = getImagesFromXrSwapchain(swapchain.swapchain);
                       return hint;
                   });
    return v;
}

std::unique_ptr<xr::impl::InputStructBase> VulkanManagerOpenxr::getXrGraphicsBinding() {
    xr::GraphicsBindingVulkanKHR graphicsBinding{};
    graphicsBinding.instance = this->vkInst;
    graphicsBinding.physicalDevice = this->vkPhysDevice;
    graphicsBinding.device = this->vkDevice;
    graphicsBinding.queueFamilyIndex = this->vkQueueSet.graphicsQueueFamilyIndex;
    graphicsBinding.queueIndex = 0;

    return std::make_unique<xr::GraphicsBindingVulkanKHR>(graphicsBinding);
}

VulkanManagerOpenxr::VulkanManagerOpenxr(xr::Instance xrInst, xr::SystemId xrSysId)
    : vkInst{createVulkanInstanceWithOpenxr(xrInst, xrSysId)},
      vkPhysDevice{getPhysicalDeviceWithOpenxr(xrInst, xrSysId, vkInst)},
      vkQueueSet{chooseSuitableQueueSet(vkPhysDevice.getQueueFamilyProperties()).value()},
      vkDevice{createDeviceWithOpenxr(xrInst, xrSysId, vkPhysDevice)},
      core{vkInst, vkPhysDevice, vkQueueSet, vkDevice} {}

void VulkanManagerOpenxr::buildRenderTarget(std::vector<OpenxrSwapchainDetails> swapchains) {
    auto hints = getRenderTargetHintsWithOpenxr(swapchains);
    core.recreateRenderTarget(hints);
}
