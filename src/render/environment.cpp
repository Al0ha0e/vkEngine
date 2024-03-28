#include <render/environment.hpp>
#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <render/descriptor.hpp>

namespace vke_render
{
    DescriptorSetAllocator *DescriptorSetAllocator::instance = nullptr;
    RenderEnvironment *RenderEnvironment::instance = nullptr;
    // using QueueFamilyIndices = RenderEnvironment::QueueFamilyIndices;
    // using SwapChainSupportDetails = RenderEnvironment::SwapChainSupportDetails;

    void RenderEnvironment::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(window_width, window_height, "Vulkan window", nullptr, nullptr);
    }

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

        return true;
    }

    void RenderEnvironment::createInstance()
    {
        if (DEBUG_MODE && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "test";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "NoEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::cout << glfwExtensionCount << " glfw extensions supported\n";

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        if (DEBUG_MODE)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &vkinstance);
    }

    void RenderEnvironment::createSurface()
    {
        if (glfwCreateWindowSurface(vkinstance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    const std::vector<const char *> deviceExtensions =
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
         VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
         VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME};

    bool checkDeviceExtensionSupport(VkPhysicalDevice pdevice)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    bool RenderEnvironment::isDeviceSuitable(VkPhysicalDevice pdevice)
    {
        QueueFamilyIndices indices = FindQueueFamilies(pdevice);
        bool extensionsSupported = checkDeviceExtensionSupport(pdevice);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(pdevice);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures2 supportedFeatures2 = {};
        supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        VkPhysicalDeviceVulkan12Features supportedFeatures12 = {};
        supportedFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        supportedFeatures2.pNext = &supportedFeatures12;
        VkPhysicalDeviceVulkan11Features supportedFeatures11 = {};
        supportedFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        supportedFeatures12.pNext = &supportedFeatures11;
        vkGetPhysicalDeviceFeatures2(pdevice, &supportedFeatures2);
        VkPhysicalDeviceFeatures &supportedFeatures = supportedFeatures2.features;

        return indices.isComplete() &&
               extensionsSupported &&
               swapChainAdequate &&
               supportedFeatures.samplerAnisotropy &&
               supportedFeatures.shaderInt64 &&
               supportedFeatures.shaderInt16 &&
               supportedFeatures.multiDrawIndirect &&
               supportedFeatures.fillModeNonSolid &&
               supportedFeatures11.storageBuffer16BitAccess &&
               supportedFeatures11.uniformAndStorageBuffer16BitAccess &&
               supportedFeatures12.shaderBufferInt64Atomics &&
               supportedFeatures12.shaderSharedInt64Atomics &&
               supportedFeatures12.shaderFloat16 &&
               supportedFeatures12.shaderInt8 &&
               supportedFeatures12.uniformAndStorageBuffer8BitAccess;
    }

    void RenderEnvironment::pickPhysicalDevice()
    {
        physicalDevice = VK_NULL_HANDLE;
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vkinstance, &deviceCount, nullptr);
        std::cout << deviceCount << " devcies \n";

        std::vector<VkPhysicalDevice> devices(deviceCount);
        std::vector<VkPhysicalDevice> candidates;
        vkEnumeratePhysicalDevices(vkinstance, &deviceCount, devices.data());

        for (const auto &device : devices)
            if (isDeviceSuitable(device))
                candidates.push_back(device);

        if (candidates.size())
        {
            std::cout << "FIND " << candidates.size() << " DEVICES\n";
            physicalDevice = candidates[0];
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void RenderEnvironment::createLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        VkPhysicalDeviceFeatures &deviceFeatures = deviceFeatures2.features;
        deviceFeatures.fillModeNonSolid = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.shaderInt64 = VK_TRUE;
        deviceFeatures.shaderInt16 = VK_TRUE;
        deviceFeatures.multiDrawIndirect = VK_TRUE;

        VkPhysicalDeviceVulkan12Features deviceFeatures12 = {};
        deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        deviceFeatures12.shaderBufferInt64Atomics = VK_TRUE;
        deviceFeatures12.shaderSharedInt64Atomics = VK_TRUE;
        deviceFeatures12.shaderFloat16 = VK_TRUE;
        deviceFeatures12.shaderInt8 = VK_TRUE;
        deviceFeatures12.uniformAndStorageBuffer8BitAccess = VK_TRUE;
        deviceFeatures2.pNext = &deviceFeatures12;

        VkPhysicalDeviceVulkan11Features deviceFeatures11 = {};
        deviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        deviceFeatures11.storageBuffer16BitAccess = VK_TRUE;
        deviceFeatures11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
        deviceFeatures12.pNext = &deviceFeatures11;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        // createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.pEnabledFeatures = nullptr;
        createInfo.pNext = &deviceFeatures2;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }
        vkGetDeviceQueue(logicalDevice, indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
        vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
    }

    void RenderEnvironment::createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();
        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void RenderEnvironment::createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void RenderEnvironment::createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
};