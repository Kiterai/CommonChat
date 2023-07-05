#include "SetupWithOpenxr.hpp"
#include "Helper.hpp"
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

xr::UniqueSwapchain createSwapchainWithOpenxr(xr::Session xrSession) {
    auto swapchainFmts = xrSession.enumerateSwapchainFormatsToVector();
    auto swapchainFmtCnt = swapchainFmts.size();

    xr::SwapchainCreateInfo swapchainCreateInfo;

    return xrSession.createSwapchainUnique(swapchainCreateInfo);
}

std::vector<vk::Image> getImagesFromXrSwapchain(xr::Swapchain swapchain) {
    auto images = swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();

    std::vector<vk::Image> vkImages;
    std::transform(images.begin(), images.end(), std::back_inserter(vkImages),
                   [](xr::SwapchainImageVulkanKHR image) { return vk::Image(image.image); });

    return vkImages;
}