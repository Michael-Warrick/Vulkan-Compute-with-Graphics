#include "application.hpp"

void Application::Run()
{
    init();
    update();
    shutdown();
}

void Application::init()
{
    initWindow();
    initVulkan();
    initImGui();
}

void Application::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1920, 1080, "Vulkan Compute with Graphics", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Application::initVulkan()
{
    createVulkanInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createTimeStampQueryPool();
    createSwapChain();
    createImageViews();
    createRenderPass();

    createGraphicsDescriptorSetLayout();
    createGraphicsPipeline();
    createComputeDescriptorSetLayout();
    createComputePipeline();

    createCommandPool();

    createColorResources();
    createDepthResources();
    createFramebuffers();

    createTextureImage(TEXTURE_PATH.c_str());
    createTextureImageView();
    createTextureSampler();

    footballModel.Load(MODEL_PATH.c_str(), physicalDevice, logicalDevice, graphicsQueue, commandPool);

    createComputeCommandPool();

    createShaderStorageBuffers();

    createUniformBuffers();
    createComputeUniformBuffers();
    createGraphicsDescriptorPool();
    createComputeDescriptorPool();
    createGraphicsDescriptorSets();
    createComputeDescriptorSets();
    createCommandBuffers();
    createComputeCommandBuffers();
    createSyncObjects();
}

void Application::update()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    logicalDevice.waitIdle();
}

void Application::shutdown()
{
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    cleanupImGui();

    cleanupSwapChain();

    logicalDevice.destroyQueryPool(queryPool);

    logicalDevice.destroyPipeline(graphicsPipeline);
    logicalDevice.destroyPipelineLayout(graphicsPipelineLayout);

    logicalDevice.destroyPipeline(computePipeline);
    logicalDevice.destroyPipelineLayout(computePipelineLayout);

    logicalDevice.destroyRenderPass(renderPass);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        logicalDevice.destroyBuffer(uniformBuffers[i]);
        logicalDevice.freeMemory(uniformBuffersMemory[i]);

        logicalDevice.destroyBuffer(computeUniformBuffers[i]);
        logicalDevice.freeMemory(computeUniformBuffersMemory[i]);
    }

    logicalDevice.destroyDescriptorPool(imguiDescriptorPool);
    logicalDevice.destroyDescriptorPool(graphicsDescriptorPool);
    logicalDevice.destroyDescriptorPool(computeDescriptorPool);

    logicalDevice.destroySampler(textureSampler);
    logicalDevice.destroyImageView(textureImageView);

    logicalDevice.destroyImage(textureImage);
    logicalDevice.freeMemory(textureImageMemory);

    logicalDevice.destroyDescriptorSetLayout(graphicsDescriptorSetLayout);
    logicalDevice.destroyDescriptorSetLayout(computeDescriptorSetLayout);

    footballModel.Destroy(logicalDevice);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        logicalDevice.destroyBuffer(shaderStorageBuffers[i]);
        logicalDevice.freeMemory(shaderStorageBuffersMemory[i]);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        logicalDevice.destroySemaphore(imageAvailableSemaphores[i]);
        logicalDevice.destroySemaphore(renderFinishedSemaphores[i]);
        logicalDevice.destroySemaphore(computeFinishedSemaphores[i]);
        logicalDevice.destroyFence(inFlightFences[i]);
        logicalDevice.destroyFence(computeInFlightFences[i]);
    }

    logicalDevice.destroyCommandPool(computeCommandPool);
    logicalDevice.destroyCommandPool(commandPool);

    logicalDevice.destroy();

    instance.destroySurfaceKHR(surface);
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::createVulkanInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Requested Validation Layers unavailable!");
    }

    if (!checkInstanceExtensionSupport())
    {
        throw std::runtime_error("Requested Instance Extensions are unavailable");
    }

    appInfo = vk::ApplicationInfo()
                  .setPApplicationName("GLFW Vulcan Example")
                  .setApplicationVersion(1)
                  .setPEngineName("No Engine")
                  .setEngineVersion(1)
                  .setApiVersion(VK_API_VERSION_1_3);

    createInfo = vk::InstanceCreateInfo()
                     .setFlags(flags)
                     .setPApplicationInfo(&appInfo)
                     .setEnabledLayerCount(validationLayers.size())
                     .setPpEnabledLayerNames(validationLayers.data())
                     .setEnabledExtensionCount(requiredExtensions.size())
                     .setPpEnabledExtensionNames(requiredExtensions.data());

    if (enableValidationLayers)
    {
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.setPNext(&debugCreateInfo);
    }

    try
    {
        instance = vk::createInstance(createInfo);
    }
    catch (const vk::SystemError &err)
    {
        throw std::runtime_error("Failed to create Vulkan instance: " + std::string(err.what()));
    }
}

bool Application::checkValidationLayerSupport()
{
    vk::Result result = vk::enumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to enumerate intance layer count. Error code: " + vk::to_string(result));
    }

    availableLayers.resize(layerCount);

    result = vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to enumerate intance available layers. Error code: " + vk::to_string(result));
    }

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    // Returning true as all validation layers were found
    return true;
}

std::vector<const char *> Application::getRequiredInstanceExtensions()
{
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        requiredExtensions.emplace_back(glfwExtensions[i]);
    }

#ifdef __APPLE__
    // Adding the portability extension (for MoltenVK driver compatibility issue) + Setting flag
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = vk::InstanceCreateFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
#endif

    if (enableValidationLayers)
    {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return requiredExtensions;
}

std::vector<vk::ExtensionProperties> Application::getAvailableInstanceExtensions()
{
    vk::Result result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to enumerate instance extension count. Error code: " + vk::to_string(result));
    }

    availableExtensions.resize(extensionCount);

    result = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to enumerate instance extension properties. Error code: " + vk::to_string(result));
    }

    return availableExtensions;
}

bool Application::checkInstanceExtensionSupport()
{
    requiredExtensions = this->getRequiredInstanceExtensions();
    availableExtensions = this->getAvailableInstanceExtensions();

    bool extensionFound;
    for (const auto &requiredExtension : requiredExtensions)
    {
        extensionFound = false;
        for (const auto &availableExtension : availableExtensions)
        {
            if (strcmp(requiredExtension, availableExtension.extensionName) == 0)
            {
                extensionFound = true;
            }
        }

        if (!extensionFound)
        {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult Application::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator,
                                                   VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Application::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                           "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, debugMessenger, pAllocator);
    }
}

void Application::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createDebugInfo)
{
    createDebugInfo = {};
    createDebugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createDebugInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createDebugInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createDebugInfo.pfnUserCallback = debugCallback;
}

void Application::setupDebugMessenger()
{
    if (!enableValidationLayers)
    {
        return;
    }

    populateDebugMessengerCreateInfo(debugCreateInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}

void Application::pickPhysicalDevice()
{
    vk::Result result = instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to enumerate physical devices. Error code: " + vk::to_string(result));
    }

    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("Failed to find a GPU with Vulkan support!");
    }

    physicalDevices = std::vector<vk::PhysicalDevice>(physicalDeviceCount);
    result = instance.enumeratePhysicalDevices(&physicalDeviceCount, physicalDevices.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Failed to enumerate physical devices with selected data. Error code: " + vk::to_string(result));
    }

    for (const auto &device : physicalDevices)
    {
        if (isDeviceSuitable(device))
        {
            physicalDevice = device;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (!physicalDevice)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool Application::checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
    uint32_t deviceExtensionCount;
    vk::Result result = device.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, nullptr);

    std::vector<vk::ExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
    result = device.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount,
                                                       availableDeviceExtensions.data());

    std::set<std::string> requiredDeviceExtensions(logicalDeviceExtensions.begin(), logicalDeviceExtensions.end());

    for (const auto &deviceExtension : availableDeviceExtensions)
    {
        requiredDeviceExtensions.erase(deviceExtension.extensionName);
    }

    return requiredDeviceExtensions.empty();
}

Application::SwapChainSupportDetails Application::querySwapChainSupport(vk::PhysicalDevice device)
{
    SwapChainSupportDetails details;
    vk::Result result = device.getSurfaceCapabilitiesKHR(surface, &details.capabilities);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to get swapchain surface capabilities! Error Code: " + vk::to_string(result));
    }

    uint32_t formatCount;
    result = device.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to get swapchain surface formats! Error Code: " + vk::to_string(result));
    }

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);

        result = device.getSurfaceFormatsKHR(surface, &formatCount, details.formats.data());
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(
                "Failed to add swapchain surface formats to swapchain support details object! Error Code: " +
                vk::to_string(result));
        }
    }

    uint32_t presentModeCount;
    result = device.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to get swapchain surface present modes! Error Code: " + vk::to_string(result));
    }

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);

        result = device.getSurfacePresentModesKHR(surface, &presentModeCount, details.presentModes.data());
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(
                "Failed to add swapchain surface present modes to swapchain support details object! Error Code: " +
                vk::to_string(result));
        }
    }

    return details;
}

bool Application::isDeviceSuitable(vk::PhysicalDevice device)
{
    Application::QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

Application::QueueFamilyIndices Application::findQueueFamilies(vk::PhysicalDevice device)
{
    Application::QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;

    device.getQueueFamilyProperties(&queueFamilyCount, nullptr);
    std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
    device.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        vk::Bool32 presentSupport = false;

        vk::Result result = device.getSurfaceSupportKHR(i, surface, &presentSupport);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to physical device surface support! Error Code: " + vk::to_string(result));
        }

        if (queueFamily.queueCount > 0)
        {
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute))
            {
                indices.graphicsAndComputeFamily = i;
            }

            if (presentSupport)
            {
                indices.presentFamily = i;
            }
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

void Application::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    float queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueFamilyCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsAndComputeFamily.value(),
        indices.presentFamily.value()};

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        queueFamilyCreateInfos.push_back(vk::DeviceQueueCreateInfo()
                                             .setQueueFamilyIndex(queueFamily)
                                             .setQueueCount(1)
                                             .setPQueuePriorities(&queuePriority));
    }

    physicalDeviceFeatures.samplerAnisotropy = vk::True;
    physicalDeviceFeatures.sampleRateShading = vk::True;

    vk::PhysicalDeviceHostQueryResetFeatures hostQueryResetFeatures = vk::PhysicalDeviceHostQueryResetFeatures()
                                                                          .setPNext(nullptr)
                                                                          .setHostQueryReset(vk::True);

    logicalDeviceCreateInfo = vk::DeviceCreateInfo()
                                  .setPQueueCreateInfos(queueFamilyCreateInfos.data())
                                  .setQueueCreateInfoCount(queueFamilyCreateInfos.size())
                                  .setPEnabledFeatures(&physicalDeviceFeatures)
                                  .setEnabledExtensionCount(logicalDeviceExtensions.size())
                                  .setPpEnabledExtensionNames(logicalDeviceExtensions.data())
                                  .setPNext(&hostQueryResetFeatures);

    if (enableValidationLayers)
    {
        logicalDeviceCreateInfo.setEnabledLayerCount(validationLayers.size());
        logicalDeviceCreateInfo.setPpEnabledLayerNames(validationLayers.data());
    }
    else
    {
        logicalDeviceCreateInfo.setEnabledLayerCount(0);
    }

    vk::Result result = physicalDevice.createDevice(&logicalDeviceCreateInfo, nullptr, &logicalDevice);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create logical device! Error Code: " + vk::to_string(result));
    }

    logicalDevice.getQueue(indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
    logicalDevice.getQueue(indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
    logicalDevice.getQueue(indices.presentFamily.value(), 0, &presentQueue);
}

void Application::createSurface()
{
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR *>(&surface));
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create surface!");
    }
}

vk::SurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Application::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    if (isVSyncEnabled)
    {
        return vk::PresentModeKHR::eFifo;
    }

    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Application::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width;
        int height;

        glfwGetFramebufferSize(window, &width, &height);
        vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Application::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainCreateInfo = vk::SwapchainCreateInfoKHR()
                                                         .setSurface(surface)
                                                         .setMinImageCount(imageCount)
                                                         .setImageFormat(surfaceFormat.format)
                                                         .setImageColorSpace(surfaceFormat.colorSpace)
                                                         .setImageExtent(extent)
                                                         .setImageArrayLayers(1)
                                                         .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                                         .setPreTransform(swapChainSupport.capabilities.currentTransform)
                                                         .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                                                         .setPresentMode(presentMode)
                                                         .setClipped(VK_TRUE)
                                                         .setOldSwapchain(VK_NULL_HANDLE);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsAndComputeFamily != indices.presentFamily)
    {
        swapChainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
        swapChainCreateInfo.setQueueFamilyIndexCount(2);
        swapChainCreateInfo.setPQueueFamilyIndices(queueFamilyIndices);
    }
    else
    {
        swapChainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        swapChainCreateInfo.setQueueFamilyIndexCount(0);
        swapChainCreateInfo.setPQueueFamilyIndices(nullptr);
    }

    vk::Result result = logicalDevice.createSwapchainKHR(&swapChainCreateInfo, nullptr, &swapChain);

    result = logicalDevice.getSwapchainImagesKHR(swapChain, &imageCount, nullptr);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to get swap chain images! Error Code: " + vk::to_string(result));
    }

    swapChainImages.resize(imageCount);

    result = logicalDevice.getSwapchainImagesKHR(swapChain, &imageCount, swapChainImages.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to set swap chain images! Error Code: " + vk::to_string(result));
    }

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Application::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
    }
}

void Application::createGraphicsPipeline()
{
    auto vertexShaderCode = readFile("resources/shaders/shader.vert.spv");
    auto fragmentShaderCode = readFile("resources/shaders/shader.frag.spv");

    vk::ShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    vk::ShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo = vk::PipelineShaderStageCreateInfo()
                                                                        .setStage(vk::ShaderStageFlagBits::eVertex)
                                                                        .setModule(vertexShaderModule)
                                                                        .setPName("main");
    vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = vk::PipelineShaderStageCreateInfo()
                                                                          .setStage(vk::ShaderStageFlagBits::eFragment)
                                                                          .setModule(fragmentShaderModule)
                                                                          .setPName("main");
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    auto bindingDescriptions = Model::Vertex::getBindingDescriptions();
    auto attributeDescriptions = Model::Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo()
                                                                       .setVertexBindingDescriptionCount(static_cast<uint32_t>(bindingDescriptions.size()))
                                                                       .setPVertexBindingDescriptions(bindingDescriptions.data())
                                                                       .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDescriptions.size()))
                                                                       .setPVertexAttributeDescriptions(attributeDescriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
                                                                           .setTopology(vk::PrimitiveTopology::eTriangleList)
                                                                           .setPrimitiveRestartEnable(VK_FALSE);

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo = vk::PipelineViewportStateCreateInfo()
                                                                      .setViewportCount(1)
                                                                      .setScissorCount(1);

    vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo = vk::PipelineRasterizationStateCreateInfo()
                                                                        .setDepthClampEnable(VK_FALSE)
                                                                        .setRasterizerDiscardEnable(VK_FALSE)
                                                                        .setPolygonMode(vk::PolygonMode::eFill)
                                                                        .setLineWidth(1.0f)
                                                                        .setCullMode(vk::CullModeFlagBits::eBack)
                                                                        .setFrontFace(vk::FrontFace::eCounterClockwise)
                                                                        .setDepthBiasEnable(VK_FALSE)
                                                                        .setDepthBiasConstantFactor(0.0f)
                                                                        .setDepthBiasClamp(0.0f)
                                                                        .setDepthBiasSlopeFactor(0.0f);

    vk::PipelineMultisampleStateCreateInfo multisamplingCreateInfo = vk::PipelineMultisampleStateCreateInfo()
                                                                         .setSampleShadingEnable(VK_FALSE)
                                                                         .setRasterizationSamples(msaaSamples)
                                                                         .setMinSampleShading(0.2f)
                                                                         .setPSampleMask(nullptr)
                                                                         .setAlphaToCoverageEnable(VK_FALSE)
                                                                         .setAlphaToOneEnable(VK_FALSE);

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
                                                                              .setDepthTestEnable(vk::True)
                                                                              .setDepthWriteEnable(vk::True)
                                                                              .setDepthCompareOp(vk::CompareOp::eLess)
                                                                              .setDepthBoundsTestEnable(vk::False)
                                                                              .setStencilTestEnable(vk::False);

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState()
                                                                          .setColorWriteMask(
                                                                              vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                                                                              vk::ColorComponentFlagBits::eA)
                                                                          .setBlendEnable(VK_FALSE)
                                                                          .setSrcColorBlendFactor(vk::BlendFactor::eOne)
                                                                          .setDstColorBlendFactor(vk::BlendFactor::eZero)
                                                                          .setColorBlendOp(vk::BlendOp::eAdd)
                                                                          .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                                                                          .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                                                                          .setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo colorBlendingCreateInfo = vk::PipelineColorBlendStateCreateInfo()
                                                                        .setLogicOpEnable(VK_FALSE)
                                                                        .setLogicOp(vk::LogicOp::eCopy)
                                                                        .setAttachmentCount(1)
                                                                        .setPAttachments(&colorBlendAttachmentState)
                                                                        .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo()
                                                                    .setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
                                                                    .setPDynamicStates(dynamicStates.data());

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                                                .setSetLayoutCount(1)
                                                                .setPSetLayouts(&graphicsDescriptorSetLayout)
                                                                .setPushConstantRangeCount(0)
                                                                .setPPushConstantRanges(nullptr);

    vk::Result result = logicalDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &graphicsPipelineLayout);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create pipeline layout! Error Code: " + vk::to_string(result));
    }

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
                                                            .setStageCount(2)
                                                            .setPStages(shaderStages)
                                                            .setPVertexInputState(&vertexInputCreateInfo)
                                                            .setPInputAssemblyState(&inputAssemblyCreateInfo)
                                                            .setPViewportState(&viewportStateCreateInfo)
                                                            .setPRasterizationState(&rasterizerCreateInfo)
                                                            .setPMultisampleState(&multisamplingCreateInfo)
                                                            .setPDepthStencilState(&depthStencilStateCreateInfo)
                                                            .setPColorBlendState(&colorBlendingCreateInfo)
                                                            .setPDynamicState(&dynamicStateCreateInfo)
                                                            .setLayout(graphicsPipelineLayout)
                                                            .setRenderPass(renderPass)
                                                            .setSubpass(0)
                                                            .setBasePipelineHandle(VK_NULL_HANDLE);

    result = logicalDevice.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create graphics pipeline! Error Code: " + vk::to_string(result));
    }

    logicalDevice.destroyShaderModule(fragmentShaderModule);
    logicalDevice.destroyShaderModule(vertexShaderModule);
}

void Application::createComputeDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                          .setBinding(0)
                                                          .setDescriptorCount(1)
                                                          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                                          .setPImmutableSamplers(nullptr)
                                                          .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutBinding lastFrameSSBOLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                                    .setBinding(1)
                                                                    .setDescriptorCount(1)
                                                                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                                                    .setPImmutableSamplers(nullptr)
                                                                    .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutBinding currentFrameSSBOLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                                       .setBinding(2)
                                                                       .setDescriptorCount(1)
                                                                       .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                                                       .setPImmutableSamplers(nullptr)
                                                                       .setStageFlags(vk::ShaderStageFlagBits::eCompute);

    std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings = {
        uboLayoutBinding,
        lastFrameSSBOLayoutBinding,
        currentFrameSSBOLayoutBinding};

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
                                                             .setBindingCount(static_cast<uint32_t>(layoutBindings.size()))
                                                             .setPBindings(layoutBindings.data());

    vk::Result result = logicalDevice.createDescriptorSetLayout(&layoutCreateInfo, nullptr, &computeDescriptorSetLayout);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor set layout! Error Code: " + vk::to_string(result));
    }
}

void Application::createComputePipeline()
{
    auto computeShaderCode = readFile("resources/shaders/shader.comp.spv");

    vk::ShaderModule computeShaderModule = createShaderModule(computeShaderCode);

    vk::PipelineShaderStageCreateInfo computeShaderStageCreateInfo = vk::PipelineShaderStageCreateInfo()
                                                                         .setStage(vk::ShaderStageFlagBits::eCompute)
                                                                         .setModule(computeShaderModule)
                                                                         .setPName("main");

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                                                .setSetLayoutCount(1)
                                                                .setPSetLayouts(&computeDescriptorSetLayout);

    vk::Result result = logicalDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &computePipelineLayout);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create compute pipeline layout! Error Code: " + vk::to_string(result));
    }

    vk::ComputePipelineCreateInfo computePipelineCreateInfo = vk::ComputePipelineCreateInfo()
                                                                  .setLayout(computePipelineLayout)
                                                                  .setStage(computeShaderStageCreateInfo);

    result = logicalDevice.createComputePipelines(nullptr, 1, &computePipelineCreateInfo, nullptr, &computePipeline);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create compute pipeline! Error Code: " + vk::to_string(result));
    }

    logicalDevice.destroyShaderModule(computeShaderModule);
}

std::vector<char> Application::readFile(const std::string &fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

vk::ShaderModule Application::createShaderModule(const std::vector<char> &code)
{
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
                                                            .setCodeSize(code.size())
                                                            .setPCode(reinterpret_cast<const uint32_t *>(code.data()));

    vk::ShaderModule shaderModule;
    vk::Result result = logicalDevice.createShaderModule(&shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create shader module! Error Code: " + vk::to_string(result));
    }

    return shaderModule;
}

void Application::createRenderPass()
{
    vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
                                                    .setFormat(swapChainImageFormat)
                                                    .setSamples(msaaSamples)
                                                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                                                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                                                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                                    .setInitialLayout(vk::ImageLayout::eUndefined)
                                                    .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
                                                           .setAttachment(0)
                                                           .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
                                                    .setFormat(findDepthFormat())
                                                    .setSamples(msaaSamples)
                                                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                                                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                                                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                                    .setInitialLayout(vk::ImageLayout::eUndefined)
                                                    .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference depthAttachmentReference = vk::AttachmentReference()
                                                           .setAttachment(1)
                                                           .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentDescription colorAttachmentResolve = vk::AttachmentDescription()
                                                           .setFormat(swapChainImageFormat)
                                                           .setSamples(vk::SampleCountFlagBits::e1)
                                                           .setLoadOp(vk::AttachmentLoadOp::eDontCare)
                                                           .setStoreOp(vk::AttachmentStoreOp::eStore)
                                                           .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                                           .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                                           .setInitialLayout(vk::ImageLayout::eUndefined)
                                                           .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentResolveReference = vk::AttachmentReference()
                                                                  .setAttachment(2)
                                                                  .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass = vk::SubpassDescription()
                                         .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                                         .setColorAttachmentCount(1)
                                         .setPColorAttachments(&colorAttachmentReference)
                                         .setPDepthStencilAttachment(&depthAttachmentReference)
                                         .setPResolveAttachments(&colorAttachmentResolveReference);

    vk::SubpassDependency subpassDependency = vk::SubpassDependency()
                                                  .setSrcSubpass(vk::SubpassExternal)
                                                  .setDstSubpass(0)
                                                  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                                                  .setSrcAccessMask(vk::AccessFlagBits::eNone)
                                                  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                                                  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
                                                        .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
                                                        .setPAttachments(attachments.data())
                                                        .setSubpassCount(1)
                                                        .setPSubpasses(&subpass)
                                                        .setDependencyCount(1)
                                                        .setPDependencies(&subpassDependency);

    vk::Result result = logicalDevice.createRenderPass(&renderPassCreateInfo, nullptr, &renderPass);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass! Error Code: " + vk::to_string(result));
    }
}

void Application::createFramebuffers()
{
    swapChainFrameBuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {

        std::array<vk::ImageView, 3> attachments = {colorImageView, depthImageView, swapChainImageViews[i]};
        vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
                                                              .setRenderPass(renderPass)
                                                              .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
                                                              .setPAttachments(attachments.data())
                                                              .setWidth(swapChainExtent.width)
                                                              .setHeight(swapChainExtent.height)
                                                              .setLayers(1);

        vk::Result result = logicalDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &swapChainFrameBuffers[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create framebuffer! Error Code: " + vk::to_string(result));
        }
    }
}

void Application::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
                                                          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                                                          .setQueueFamilyIndex(queueFamilyIndices.graphicsAndComputeFamily.value());

    vk::Result result = logicalDevice.createCommandPool(&commandPoolCreateInfo, nullptr, &commandPool);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create command pool! Error Code: " + vk::to_string(result));
    }
}

void Application::createComputeCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
                                                          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                                                          .setQueueFamilyIndex(queueFamilyIndices.graphicsAndComputeFamily.value());

    vk::Result result = logicalDevice.createCommandPool(&commandPoolCreateInfo, nullptr, &computeCommandPool);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create command pool! Error Code: " + vk::to_string(result));
    }
}

void Application::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocateCreateInfo = vk::CommandBufferAllocateInfo()
                                                           .setCommandPool(commandPool)
                                                           .setLevel(vk::CommandBufferLevel::ePrimary)
                                                           .setCommandBufferCount((uint32_t)commandBuffers.size());

    vk::Result result = logicalDevice.allocateCommandBuffers(&allocateCreateInfo, commandBuffers.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffers! Error Code: " + vk::to_string(result));
    }
}

void Application::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    vk::CommandBufferBeginInfo beginCreateInfo = vk::CommandBufferBeginInfo();

    vk::Result result = commandBuffer.begin(&beginCreateInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffers! Error Code: " + vk::to_string(result));
    }

    commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool, 2);

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

    vk::RenderPassBeginInfo renderPassBeginCreateInfo = vk::RenderPassBeginInfo()
                                                            .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
                                                            .setPClearValues(clearValues.data())
                                                            .setRenderPass(renderPass)
                                                            .setFramebuffer(swapChainFrameBuffers[imageIndex])
                                                            .setRenderArea(vk::Rect2D({0, 0}, swapChainExtent));

    commandBuffer.beginRenderPass(&renderPassBeginCreateInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::Viewport viewport = vk::Viewport()
                                .setX(0.0f)
                                .setY(0.0f)
                                .setWidth((float)swapChainExtent.width)
                                .setHeight((float)swapChainExtent.height)
                                .setMinDepth(0.0f)
                                .setMaxDepth(1.0f);
    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor = vk::Rect2D()
                             .setOffset({0, 0})
                             .setExtent(swapChainExtent);
    commandBuffer.setScissor(0, 1, &scissor);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipelineLayout, 0, 1,
                                     &graphicsDescriptorSets[currentFrame], 0, nullptr);

    footballModel.DrawInstanced(commandBuffer, PHYSICS_OBJECT_COUNT);

    drawUI(commandBuffer);

    commandBuffer.endRenderPass();

    commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 3);
    commandBuffer.end();
}

void Application::drawFrame()
{
    // Compute submission
    vk::Result result = logicalDevice.waitForFences(1, &computeInFlightFences[currentFrame], vk::True, UINT64_MAX);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for in-flight fence! Error Code: " + vk::to_string(result));
    }

    updateComputeUniformBuffer(currentFrame);

    result = logicalDevice.resetFences(1, &computeInFlightFences[currentFrame]);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to reset in-flight fence! Error Code: " + vk::to_string(result));
    }

    computeCommandBuffers[currentFrame].reset();
    recordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setCommandBufferCount(1)
                                    .setPCommandBuffers(&computeCommandBuffers[currentFrame])
                                    .setSignalSemaphoreCount(1)
                                    .setPSignalSemaphores(&computeFinishedSemaphores[currentFrame]);

    result = computeQueue.submit(1, &submitInfo, computeInFlightFences[currentFrame]);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit compute command buffer! Error Code: " + vk::to_string(result));
    }

    // Graphics submission
    result = logicalDevice.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for in-flight fence! Error Code: " + vk::to_string(result));
    }

    uint32_t imageIndex;
    result = logicalDevice.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                               VK_NULL_HANDLE, &imageIndex);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame);

    result = logicalDevice.resetFences(1, &inFlightFences[currentFrame]);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to reset in-flight fence! Error Code: " + vk::to_string(result));
    }

    commandBuffers[currentFrame].reset();
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    vk::Semaphore waitSemaphores[] = {computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    submitInfo = vk::SubmitInfo()
                     .setWaitSemaphoreCount(2)
                     .setPWaitSemaphores(waitSemaphores)
                     .setPWaitDstStageMask(waitStages)
                     .setCommandBufferCount(1)
                     .setPCommandBuffers(&commandBuffers[currentFrame])
                     .setSignalSemaphoreCount(1)
                     .setPSignalSemaphores(signalSemaphores);

    result = graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit draw command buffer! Error Code: " + vk::to_string(result));
    }

    vk::SwapchainKHR swapChains[] = {swapChain};

    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
                                         .setWaitSemaphoreCount(1)
                                         .setPWaitSemaphores(signalSemaphores)
                                         .setSwapchainCount(1)
                                         .setPSwapchains(swapChains)
                                         .setPImageIndices(&imageIndex)
                                         .setPResults(nullptr);

    result = presentQueue.presentKHR(&presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present: present queue! Error Code: " + vk::to_string(result));
    }

    getTimeStampResults();

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreCreateInfo = vk::SemaphoreCreateInfo();
    vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo()
                                              .setFlags(vk::FenceCreateFlagBits::eSignaled);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::Result result = logicalDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(
                "Failed to create image available semaphore! Error Code: " + vk::to_string(result));
        }

        result = logicalDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(
                "Failed to create render finished semaphore! Error Code: " + vk::to_string(result));
        }

        result = logicalDevice.createFence(&fenceCreateInfo, nullptr, &inFlightFences[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create in-flight fence! Error Code: " + vk::to_string(result));
        }

        result = logicalDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &computeFinishedSemaphores[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(
                "failed to create compute synchronization objects for a frame! Error Code: " + vk::to_string(result));
        }

        result = logicalDevice.createFence(&fenceCreateInfo, nullptr, &computeInFlightFences[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create compute synchronization objects for a frame  ! Error Code: " + vk::to_string(result));
        }
    }
}

void Application::recreateSwapChain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    logicalDevice.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
}

void Application::cleanupSwapChain()
{
    logicalDevice.destroyImageView(colorImageView);
    logicalDevice.destroyImage(colorImage);
    logicalDevice.freeMemory(colorImageMemory);

    logicalDevice.destroyImageView(depthImageView);
    logicalDevice.destroyImage(depthImage);
    logicalDevice.freeMemory(depthImageMemory);

    for (auto swapChainFrameBuffer : swapChainFrameBuffers)
    {
        logicalDevice.destroyFramebuffer(swapChainFrameBuffer);
    }

    for (auto swapChainImageView : swapChainImageViews)
    {
        logicalDevice.destroyImageView(swapChainImageView);
    }

    logicalDevice.destroySwapchainKHR(swapChain);
}

void Application::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

uint32_t Application::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    physicalDevice.getMemoryProperties(&physicalDeviceMemoryProperties);

    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void Application::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                               vk::Buffer &buffer, vk::DeviceMemory &bufferMemory)
{
    vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
                                                .setSize(size)
                                                .setUsage(usage)
                                                .setSharingMode(vk::SharingMode::eExclusive);

    vk::Result result = logicalDevice.createBuffer(&bufferCreateInfo, nullptr, &buffer);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create buffer! Error Code: " + vk::to_string(result));
    }

    vk::MemoryRequirements memoryRequirements;
    logicalDevice.getBufferMemoryRequirements(buffer, &memoryRequirements);

    vk::MemoryAllocateInfo memoryAllocateInfo = vk::MemoryAllocateInfo()
                                                    .setAllocationSize(memoryRequirements.size)
                                                    .setMemoryTypeIndex(findMemoryType(memoryRequirements.memoryTypeBits, properties));

    result = logicalDevice.allocateMemory(&memoryAllocateInfo, nullptr, &bufferMemory);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate buffer memory! Error Code: " + vk::to_string(result));
    }

    logicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

void Application::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    vk::BufferCopy copyRegion = vk::BufferCopy().setSize(size);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, commandPool);
}

void Application::createGraphicsDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                          .setBinding(0)
                                                          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                                          .setDescriptorCount(1)
                                                          .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                                                          .setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding samplerLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                              .setBinding(1)
                                                              .setDescriptorCount(1)
                                                              .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                                                              .setPImmutableSamplers(nullptr)
                                                              .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    vk::DescriptorSetLayoutBinding ssboLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                           .setBinding(2)
                                                           .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                                           .setDescriptorCount(1)
                                                           .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                                                           .setPImmutableSamplers(nullptr);

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, samplerLayoutBinding, ssboLayoutBinding};
    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
                                                             .setBindingCount(static_cast<uint32_t>(bindings.size()))
                                                             .setPBindings(bindings.data());

    vk::Result result = logicalDevice.createDescriptorSetLayout(&layoutCreateInfo, nullptr, &graphicsDescriptorSetLayout);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor set layout! Error Code: " + vk::to_string(result));
    }
}

std::vector<Application::PhysicsObject> Application::createSphereBox(uint32_t objectCount, float sphereRadius)
{
    // Calculate the size of the box to fit all spheres in a cube
    uint32_t boxSize = std::ceil(std::cbrt(objectCount));
    std::vector<PhysicsObject> physicsObjects(objectCount);

    // Calculate box dimensions based on sphere radius and count
    float boxWidth = (boxSize - 1) * sphereRadius * 2.0f;
    float boxHeight = (boxSize - 1) * sphereRadius * 2.0f;
    float boxDepth = (boxSize - 1) * sphereRadius * 2.0f;

    // Iterate through the grid and create physics objects
    int index = 0;
    for (uint32_t x = 0; x < boxSize && index < objectCount; ++x)
    {
        for (uint32_t y = 0; y < boxSize && index < objectCount; ++y)
        {
            for (uint32_t z = 0; z < boxSize && index < objectCount; ++z)
            {
                // Calculate sphere position
                float xPos = -boxWidth / 2.0f + x * sphereRadius * 2.0f + sphereRadius;
                float yPos = -boxHeight / 2.0f + y * sphereRadius * 2.0f + sphereRadius;
                float zPos = -boxDepth / 2.0f + z * sphereRadius * 2.0f + sphereRadius;

                // Initialize physics object
                PhysicsObject obj;
                obj.position = glm::vec3(xPos, yPos + 2.0f, zPos);
                obj.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                obj.velocity = glm::vec3(0.0f);
                obj.angularVelocity = glm::vec3(0.0f);
                obj.radius = sphereRadius;
                obj.mass = 0.45f;
                obj.elasticity = 0.8f;
                obj.momentOfInertia = (2.0f / 5.0f) * obj.mass * (obj.radius * obj.radius);

                physicsObjects[index] = obj;
                index++;
            }
        }
    }

    return physicsObjects;
}

void Application::createShaderStorageBuffers()
{
    std::vector<PhysicsObject> objects = createSphereBox(PHYSICS_OBJECT_COUNT, 0.115f);

    vk::DeviceSize bufferSize = sizeof(PhysicsObject) * PHYSICS_OBJECT_COUNT;

    // Creating a staging buffer to upload data to the GPU
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostVisible, stagingBuffer, stagingBufferMemory);

    void *data;
    vk::Result result = logicalDevice.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map uniform buffer memory! Error Code: " + vk::to_string(result));
    }

    memcpy(data, objects.data(), (size_t)bufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    // Copy initial particle data to all storage buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
        copyBuffer(stagingBuffer, shaderStorageBuffers[i], bufferSize);
    }

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}

void Application::createUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    vk::Result result;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     uniformBuffers[i], uniformBuffersMemory[i]);
        result = logicalDevice.mapMemory(uniformBuffersMemory[i], 0, bufferSize, vk::MemoryMapFlags(),
                                         &uniformBuffersMapped[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to map graphics uniform buffer memory! Error Code: " + vk::to_string(result));
        }
    }
}

void Application::createComputeUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(ComputeUniformBufferObject);

    computeUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    computeUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    computeUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    vk::Result result;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     computeUniformBuffers[i], computeUniformBuffersMemory[i]);
        result = logicalDevice.mapMemory(computeUniformBuffersMemory[i], 0, bufferSize, vk::MemoryMapFlags(),
                                         &computeUniformBuffersMapped[i]);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to map compute uniform buffer memory! Error Code: " + vk::to_string(result));
        }
    }
}

void Application::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    glm::vec3 cameraPosition(0.0f, 5.0f, -5.0f);
    glm::vec3 cameraLookPosition(0.0f, 2.5f, 0.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

    float fov = 90.0f;
    float aspectRatio = swapChainExtent.width / (float)swapChainExtent.height;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    UniformBufferObject ubo;
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::lookAt(cameraPosition, cameraLookPosition, cameraUp);
    ubo.projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);

    // Flipping Y axis to comply with Vulkan's -1:1 viewport mapping
    ubo.projection[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Application::updateComputeUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    startTime = currentTime;

    ComputeUniformBufferObject computeUBO;
    computeUBO.physicsTimeStep = deltaTime;

    memcpy(computeUniformBuffersMapped[currentImage], &computeUBO, sizeof(computeUBO));
}

void Application::createGraphicsDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 3> poolSizes{};
    poolSizes[0] = vk::DescriptorPoolSize()
                       .setType(vk::DescriptorType::eUniformBuffer)
                       .setDescriptorCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    poolSizes[1] = vk::DescriptorPoolSize()
                       .setType(vk::DescriptorType::eCombinedImageSampler)
                       .setDescriptorCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

    // Needed so that the vertex shader can access the SSBO output buffer
    poolSizes[2] = vk::DescriptorPoolSize()
                       .setType(vk::DescriptorType::eStorageBuffer)
                       .setDescriptorCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

    vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
                                                      .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
                                                      .setPPoolSizes(poolSizes.data())
                                                      .setMaxSets(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

    vk::Result result = logicalDevice.createDescriptorPool(&poolCreateInfo, nullptr, &graphicsDescriptorPool);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor pool! Error Code: " + vk::to_string(result));
    }
}

void Application::createComputeDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes;
    poolSizes[0] = vk::DescriptorPoolSize()
                       .setType(vk::DescriptorType::eUniformBuffer)
                       .setDescriptorCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    poolSizes[1] = vk::DescriptorPoolSize()
                       .setType(vk::DescriptorType::eStorageBuffer)
                       .setDescriptorCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2);

    vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
                                                      .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
                                                      .setPPoolSizes(poolSizes.data())
                                                      .setMaxSets(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

    vk::Result result = logicalDevice.createDescriptorPool(&poolCreateInfo, nullptr, &computeDescriptorPool);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create compute descriptor pool! Error Code: " + vk::to_string(result));
    }
}

void Application::createGraphicsDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, graphicsDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
                                                     .setDescriptorPool(graphicsDescriptorPool)
                                                     .setDescriptorSetCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT))
                                                     .setPSetLayouts(layouts.data());

    graphicsDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    vk::Result result = logicalDevice.allocateDescriptorSets(&allocateInfo, graphicsDescriptorSets.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate descriptor sets! Error Code: " + vk::to_string(result));
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo()
                                                  .setBuffer(uniformBuffers[i])
                                                  .setOffset(0)
                                                  .setRange(sizeof(UniformBufferObject));

        vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
                                                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                                                .setImageView(textureImageView)
                                                .setSampler(textureSampler);

        vk::DescriptorBufferInfo ssboBufferInfo = vk::DescriptorBufferInfo()
                                                      .setBuffer(shaderStorageBuffers[i])
                                                      .setOffset(0)
                                                      .setRange(vk::WholeSize);

        std::array<vk::WriteDescriptorSet, 3> descriptorWrites{};

        descriptorWrites[0] = vk::WriteDescriptorSet()
                                  .setDstSet(graphicsDescriptorSets[i])
                                  .setDstBinding(0)
                                  .setDstArrayElement(0)
                                  .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                  .setDescriptorCount(1)
                                  .setPBufferInfo(&bufferInfo);

        descriptorWrites[1] = vk::WriteDescriptorSet()
                                  .setDstSet(graphicsDescriptorSets[i])
                                  .setDstBinding(1)
                                  .setDstArrayElement(0)
                                  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                                  .setDescriptorCount(1)
                                  .setPImageInfo(&imageInfo);

        descriptorWrites[2] = vk::WriteDescriptorSet()
                                  .setDstSet(graphicsDescriptorSets[i])
                                  .setDstBinding(2)
                                  .setDstArrayElement(0)
                                  .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                  .setDescriptorCount(1)
                                  .setPBufferInfo(&ssboBufferInfo);

        logicalDevice.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Application::createTextureImage(const char *texturePath)
{
    int textureWidth;
    int textureHeight;
    int textureChannels;

    stbi_uc *pixels = stbi_load(texturePath, &textureWidth, &textureHeight, &textureChannels,
                                STBI_rgb_alpha);
    vk::DeviceSize imageSize = textureWidth * textureHeight * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vk::Result result = logicalDevice.mapMemory(stagingBufferMemory, 0, imageSize, vk::MemoryMapFlags(), &data);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map image texture buffer memory! Error Code: " + vk::to_string(result));
    }

    memcpy(data, pixels, static_cast<size_t>(imageSize));

    logicalDevice.unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(textureWidth, textureHeight, mipLevels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);

    generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, textureWidth, textureHeight, mipLevels);
}

void Application::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples,
                              vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                              vk::MemoryPropertyFlags properties, vk::Image &image, vk::DeviceMemory &imageMemory)
{
    vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
                                              .setImageType(vk::ImageType::e2D)
                                              .setExtent(
                                                  vk::Extent3D()
                                                      .setWidth(width)
                                                      .setHeight(height)
                                                      .setDepth(1))
                                              .setMipLevels(mipLevels)
                                              .setArrayLayers(1)
                                              .setFormat(format)
                                              .setTiling(tiling)
                                              .setInitialLayout(vk::ImageLayout::eUndefined)
                                              .setUsage(usage)
                                              .setSamples(numSamples)
                                              .setSharingMode(vk::SharingMode::eExclusive);

    vk::Result result = logicalDevice.createImage(&imageCreateInfo, nullptr, &image);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create image! Error Code: " + vk::to_string(result));
    }

    vk::MemoryRequirements memoryRequirements;
    logicalDevice.getImageMemoryRequirements(image, &memoryRequirements);

    vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo()
                                              .setAllocationSize(memoryRequirements.size)
                                              .setMemoryTypeIndex(findMemoryType(memoryRequirements.memoryTypeBits, properties));

    result = logicalDevice.allocateMemory(&allocateInfo, nullptr, &imageMemory);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate image memory! Error Code: " + vk::to_string(result));
    }

    logicalDevice.bindImageMemory(image, imageMemory, 0);
}

vk::CommandBuffer Application::beginSingleTimeCommands(vk::CommandPool commandPool)
{
    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
                                                     .setLevel(vk::CommandBufferLevel::ePrimary)
                                                     .setCommandPool(commandPool)
                                                     .setCommandBufferCount(1);

    vk::CommandBuffer commandBuffer;
    vk::Result result = logicalDevice.allocateCommandBuffers(&allocateInfo, &commandBuffer);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffers! Error Code: " + vk::to_string(result));
    }

    vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
                                                            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    result = commandBuffer.begin(&commandBufferBeginInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to begin command buffer! Error Code: " + vk::to_string(result));
    }

    return commandBuffer;
}

void Application::endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::CommandPool commandPool)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setCommandBufferCount(1)
                                    .setPCommandBuffers(&commandBuffer);

    vk::Result result = graphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command buffer to graphics queue! Error Code: " + vk::to_string(result));
    }
    graphicsQueue.waitIdle();

    logicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void Application::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    vk::ImageMemoryBarrier imageMemoryBarrier = vk::ImageMemoryBarrier()
                                                    .setOldLayout(oldLayout)
                                                    .setNewLayout(newLayout)
                                                    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                                    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                                    .setImage(image)
                                                    .setSubresourceRange(
                                                        vk::ImageSubresourceRange()
                                                            .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                            .setBaseMipLevel(0)
                                                            .setLevelCount(mipLevels)
                                                            .setBaseArrayLayer(0)
                                                            .setLayerCount(1));

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage,
                                  vk::DependencyFlags(),
                                  0, nullptr,
                                  0, nullptr,
                                  1, &imageMemoryBarrier);

    endSingleTimeCommands(commandBuffer, commandPool);
}

void Application::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    vk::BufferImageCopy region = vk::BufferImageCopy()
                                     .setBufferOffset(0)
                                     .setBufferRowLength(0)
                                     .setBufferImageHeight(0)
                                     .setImageSubresource(
                                         vk::ImageSubresourceLayers()
                                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                             .setMipLevel(0)
                                             .setBaseArrayLayer(0)
                                             .setLayerCount(1))
                                     .setImageOffset(vk::Offset3D(0, 0, 0))
                                     .setImageExtent(vk::Extent3D(width, height, 1));

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
    endSingleTimeCommands(commandBuffer, commandPool);
}

vk::ImageView Application::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo()
                                                      .setImage(image)
                                                      .setViewType(vk::ImageViewType::e2D)
                                                      .setFormat(format)
                                                      .setSubresourceRange(vk::ImageSubresourceRange(
                                                          aspectFlags,
                                                          0, mipLevels, 0, 1));

    vk::ImageView imageView;
    vk::Result result = logicalDevice.createImageView(&imageViewCreateInfo, nullptr, &imageView);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create texture image view!");
    }

    return imageView;
}

void Application::createTextureImageView()
{
    textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
}

void Application::createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

    vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
                                                  .setMagFilter(vk::Filter::eLinear)
                                                  .setMinFilter(vk::Filter::eLinear)
                                                  .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                                                  .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                                                  .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                                                  .setAnisotropyEnable(vk::True)
                                                  .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
                                                  .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                                                  .setUnnormalizedCoordinates(vk::False)
                                                  .setCompareEnable(vk::False)
                                                  .setCompareOp(vk::CompareOp::eAlways)
                                                  .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                                  .setMipLodBias(0.0f)
                                                  .setMinLod(0.0f)
                                                  .setMaxLod(static_cast<float>(mipLevels));

    vk::Result result = logicalDevice.createSampler(&samplerCreateInfo, nullptr, &textureSampler);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

void Application::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage,
                depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
}

vk::Format Application::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        vk::FormatProperties properties;
        physicalDevice.getFormatProperties(format, &properties);

        if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

vk::Format Application::findDepthFormat()
{
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool Application::hasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

// Create a way to pre-generate mipmap levels as a way to cache them for faster runtime texture loading...
void Application::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels)
{
    // Check if linear blitting is supported
    vk::FormatProperties formatProperties;
    physicalDevice.getFormatProperties(imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
                                         .setImage(image)
                                         .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                         .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                         .setSubresourceRange(
                                             vk::ImageSubresourceRange()
                                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                 .setBaseArrayLayer(0)
                                                 .setLayerCount(1)
                                                 .setLevelCount(1));

    int32_t mipWidth = textureWidth;
    int32_t mipHeight = textureHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.setBaseMipLevel(i - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                                      vk::DependencyFlags(),
                                      0, nullptr,
                                      0, nullptr,
                                      1, &barrier);

        vk::ImageBlit blit = vk::ImageBlit()
                                 .setSrcOffsets({vk::Offset3D(0, 0, 0),
                                                 vk::Offset3D(mipWidth, mipHeight, 1)})
                                 .setSrcSubresource(vk::ImageSubresourceLayers(
                                     vk::ImageAspectFlagBits::eColor,
                                     static_cast<uint32_t>(i - 1),
                                     0,
                                     1))
                                 .setDstOffsets({vk::Offset3D(0, 0, 0),
                                                 vk::Offset3D(
                                                     mipWidth > 1 ? mipWidth / 2 : 1,
                                                     mipHeight > 1 ? mipHeight / 2 : 1,
                                                     1)})
                                 .setDstSubresource(vk::ImageSubresourceLayers(
                                     vk::ImageAspectFlagBits::eColor,
                                     static_cast<uint32_t>(i),
                                     0,
                                     1));

        commandBuffer.blitImage(
            image, vk::ImageLayout::eTransferSrcOptimal,
            image, vk::ImageLayout::eTransferDstOptimal,
            1, &blit, vk::Filter::eLinear);

        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                      vk::DependencyFlags(),
                                      0, nullptr,
                                      0, nullptr,
                                      1, &barrier);

        if (mipWidth > 1)
        {
            mipWidth /= 2;
        }
        if (mipHeight > 1)
        {
            mipHeight /= 2;
        }
    }

    barrier.subresourceRange.setBaseMipLevel(mipLevels - 1);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                  vk::DependencyFlags(),
                                  0, nullptr,
                                  0, nullptr,
                                  1, &barrier);

    endSingleTimeCommands(commandBuffer, commandPool);
}

vk::SampleCountFlagBits Application::getMaxUsableSampleCount()
{
    vk::PhysicalDeviceProperties physicalDevicesProperties;
    physicalDevice.getProperties(&physicalDevicesProperties);

    vk::SampleCountFlags sampleCounts = (physicalDevicesProperties.limits.framebufferColorSampleCounts & physicalDevicesProperties.limits.framebufferDepthSampleCounts);
    if (sampleCounts & vk::SampleCountFlagBits::e64)
    {
        return vk::SampleCountFlagBits::e64;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e32)
    {
        return vk::SampleCountFlagBits::e32;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e16)
    {
        return vk::SampleCountFlagBits::e16;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e8)
    {
        return vk::SampleCountFlagBits::e8;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e4)
    {
        return vk::SampleCountFlagBits::e4;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e2)
    {
        return vk::SampleCountFlagBits::e2;
    }

    return vk::SampleCountFlagBits::e1;
}

void Application::createColorResources()
{
    vk::Format colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                colorImage, colorImageMemory);

    colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
}

void Application::createComputeDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo()
                                                     .setDescriptorPool(computeDescriptorPool)
                                                     .setDescriptorSetCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT))
                                                     .setPSetLayouts(layouts.data());

    computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    vk::Result result = logicalDevice.allocateDescriptorSets(&allocateInfo, computeDescriptorSets.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate compute descriptor sets! Error Code: " + vk::to_string(result));
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo uniformBufferInfo = vk::DescriptorBufferInfo()
                                                         .setBuffer(computeUniformBuffers[i])
                                                         .setOffset(0)
                                                         .setRange(sizeof(ComputeUniformBufferObject));

        vk::DescriptorBufferInfo storageBufferInfoLastFrame = vk::DescriptorBufferInfo()
                                                                  .setBuffer(shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT])
                                                                  .setOffset(0)
                                                                  .setRange(sizeof(PhysicsObject) * PHYSICS_OBJECT_COUNT);

        vk::DescriptorBufferInfo storageBufferInfoCurrentFrame = vk::DescriptorBufferInfo()
                                                                     .setBuffer(shaderStorageBuffers[i])
                                                                     .setOffset(0)
                                                                     .setRange(sizeof(PhysicsObject) * PHYSICS_OBJECT_COUNT);

        std::array<vk::WriteDescriptorSet, 3> descriptorWrites;
        descriptorWrites[0] = vk::WriteDescriptorSet()
                                  .setDstSet(computeDescriptorSets[i])
                                  .setDstBinding(0)
                                  .setDstArrayElement(0)
                                  .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                  .setDescriptorCount(1)
                                  .setPBufferInfo(&uniformBufferInfo);

        descriptorWrites[1] = vk::WriteDescriptorSet()
                                  .setDstSet(computeDescriptorSets[i])
                                  .setDstBinding(1)
                                  .setDstArrayElement(0)
                                  .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                  .setDescriptorCount(1)
                                  .setPBufferInfo(&storageBufferInfoLastFrame);

        descriptorWrites[2] = vk::WriteDescriptorSet()
                                  .setDstSet(computeDescriptorSets[i])
                                  .setDstBinding(2)
                                  .setDstArrayElement(0)
                                  .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                  .setDescriptorCount(1)
                                  .setPBufferInfo(&storageBufferInfoCurrentFrame);

        logicalDevice.updateDescriptorSets(3, descriptorWrites.data(), 0, nullptr);
    }
}

void Application::createComputeCommandBuffers()
{
    computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
                                                                  .setCommandPool(computeCommandPool)
                                                                  .setLevel(vk::CommandBufferLevel::ePrimary)
                                                                  .setCommandBufferCount((uint32_t)computeCommandBuffers.size());

    vk::Result result = logicalDevice.allocateCommandBuffers(&commandBufferAllocateInfo, computeCommandBuffers.data());
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate compute command buffers! Error Code: " + vk::to_string(result));
    }
}

void Application::recordComputeCommandBuffer(vk::CommandBuffer commandBuffer)
{
    vk::CommandBufferBeginInfo commandBufferBeginInfo;

    vk::Result result = commandBuffer.begin(&commandBufferBeginInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to begin recording compute command buffer! Error Code: " + vk::to_string(result));
    }
    logicalDevice.resetQueryPool(queryPool, 0, static_cast<uint32_t>(timeStamps.size()));
    commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool, 0);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipelineLayout, 0, 1, &computeDescriptorSets[currentFrame], 0, nullptr);
    commandBuffer.dispatch(PHYSICS_OBJECT_COUNT / WORKGROUP_SIZE_X, 1, 1);

    commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, 1);

    commandBuffer.end();
}

std::string Application::formatIntStringWithCommas(int number)
{
    std::string numberString = std::to_string(number);
    int commaLocation = numberString.length() - 3;

    while (commaLocation > 0)
    {
        numberString.insert(commaLocation, ",");
        commaLocation -= 3;
    }

    return numberString;
}

void Application::initImGui()
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().Fonts->AddFontFromFileTTF("../resources/fonts/RobotoMono/RobotoMono-Regular.ttf", 26.0f);

    ImGui_ImplGlfw_InitForVulkan(window, true);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    createImGuiDescriptorPool();

    ImGui_ImplVulkan_InitInfo imguiInitInfo = {};
    imguiInitInfo.Instance = instance;
    imguiInitInfo.PhysicalDevice = physicalDevice;
    imguiInitInfo.Device = logicalDevice;
    imguiInitInfo.QueueFamily = indices.graphicsAndComputeFamily.value();
    imguiInitInfo.Queue = graphicsQueue;
    imguiInitInfo.PipelineCache = nullptr;
    imguiInitInfo.DescriptorPool = imguiDescriptorPool;
    imguiInitInfo.RenderPass = renderPass;
    imguiInitInfo.Subpass = 0;
    imguiInitInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    imguiInitInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
    imguiInitInfo.MSAASamples = (VkSampleCountFlagBits)msaaSamples;
    imguiInitInfo.Allocator = nullptr;
    imguiInitInfo.CheckVkResultFn = nullptr;

    ImGui_ImplVulkan_Init(&imguiInitInfo);

    objectString = "Number of Physics Objects: " + formatIntStringWithCommas(PHYSICS_OBJECT_COUNT);
}

void Application::createImGuiDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 11> pool_sizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment, 1000}};

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo = vk::DescriptorPoolCreateInfo()
                                                                .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                                                                .setMaxSets(1000 * static_cast<uint32_t>(pool_sizes.size()))
                                                                .setPoolSizeCount(static_cast<uint32_t>(pool_sizes.size()))
                                                                .setPPoolSizes(pool_sizes.data());

    imguiDescriptorPool = logicalDevice.createDescriptorPool(descriptorPoolCreateInfo);
}

void Application::drawOverlay()
{
    bool showWindow = true;
    static int location = 0;
    ImGuiIO &io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav;
    const float PAD = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;

    window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
    window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
    window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
    window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

    if (ImGui::Begin("Example: Simple overlay", &showWindow, window_flags))
    {
        ImGui::Text("PROFILER");
        ImGui::Separator();
        ImGui::Text(objectString.c_str());
        ImGui::Separator();
        ImGui::Text("Compute:                   %.3f ms", computePipelineTimeMS);
        ImGui::Text("Graphics:                  %.3f ms", graphicsPipelineTimeMS);
        ImGui::Text("Application:               %.3f ms", 1000.0f / io.Framerate);
        ImGui::Separator();
        ImGui::Text("Framerate:                 %.1f FPS", io.Framerate);
    }
    ImGui::End();
}

void Application::drawUI(vk::CommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    drawOverlay();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, nullptr);
}

void Application::cleanupImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::createTimeStampQueryPool()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    if (properties.limits.timestampPeriod == 0)
    {
        throw std::runtime_error("Timestamp queries are not supported for this device!");
    }

    if (!properties.limits.timestampComputeAndGraphics)
    {
        // Check if the graphics queue used in this sample supports time stamps
        auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        if (queueFamilyProperties[indices.graphicsAndComputeFamily.value()].timestampValidBits == 0)
        {
            throw std::runtime_error{"The selected graphics queue family does not support timestamp queries!"};
        }
    }

    timeStamps.resize(4);

    vk::QueryPoolCreateInfo queryPoolCreateInfo = vk::QueryPoolCreateInfo()
                                                      .setQueryType(vk::QueryType::eTimestamp)
                                                      .setQueryCount(static_cast<uint32_t>(timeStamps.size()));

    vk::Result result = logicalDevice.createQueryPool(&queryPoolCreateInfo, nullptr, &queryPool);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create query pool. Error code: " + vk::to_string(result));
    }
}

void Application::getTimeStampResults()
{
    vk::Result result = logicalDevice.getQueryPoolResults(
        queryPool,
        0,
        static_cast<uint32_t>(timeStamps.size()),
        timeStamps.size() * sizeof(uint64_t),
        timeStamps.data(),
        sizeof(uint64_t),
        vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);

    vk::PhysicalDeviceLimits const &physicalDeviceLimits = physicalDevice.getProperties().limits;
    computePipelineTimeMS = float(timeStamps[1] - timeStamps[0]) * physicalDeviceLimits.timestampPeriod / 1'000'000.0f;
    graphicsPipelineTimeMS = float(timeStamps[3] - timeStamps[2]) * physicalDeviceLimits.timestampPeriod / 1'000'000.0f;
}