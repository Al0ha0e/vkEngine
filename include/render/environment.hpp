#ifndef ENV_H
#define ENV_H

#include <common.hpp>
#include <optional>
#include <vector>
#include <iostream>
#include <map>
#include <memory>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace vke_render
{
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsAndComputeFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete()
        {
            return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class RenderEnvironment
    {
    private:
        static RenderEnvironment *instance;
        RenderEnvironment() {}
        ~RenderEnvironment() {}
        RenderEnvironment(const RenderEnvironment &);
        RenderEnvironment &operator=(const RenderEnvironment);

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (RenderEnvironment::instance != nullptr)
                    delete RenderEnvironment::instance;
            }
        };
        static Deletor deletor;

    public:
        static RenderEnvironment *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("RenderEnvironment not initialized!");
            return instance;
        }

        static RenderEnvironment *Init(int width, int height)
        {
            instance = new RenderEnvironment();
            instance->window_width = width;
            instance->window_height = height;
            instance->initWindow();
            instance->createInstance();
            instance->createSurface();
            instance->pickPhysicalDevice();
            instance->createLogicalDevice();
            instance->createCommandPool();
            instance->createCommandBuffers();
            instance->createSyncObjects();
            return instance;
        }

        static void Dispose()
        {
            VkDevice logicalDevice = instance->logicalDevice;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vkDestroySemaphore(logicalDevice, instance->renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(logicalDevice, instance->imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(logicalDevice, instance->inFlightFences[i], nullptr);
            }
            vkDestroyCommandPool(logicalDevice, instance->commandPool, nullptr);

            vkDestroyDevice(logicalDevice, nullptr);
            vkDestroySurfaceKHR(instance->vkinstance, instance->surface, nullptr);
            vkDestroyInstance(instance->vkinstance, nullptr);
            glfwTerminate();
        }

        static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(instance->physicalDevice, &memProperties);
            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;

            throw std::runtime_error("failed to find suitable memory type!");
        }

        static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkDevice logicalDevice = instance->logicalDevice;

            if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer!");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
        }

        static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            EndSingleTimeCommands(commandBuffer);
        }

        static void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(instance->logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(instance->logicalDevice, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(instance->logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate image memory!");
            }

            vkBindImageMemory(instance->logicalDevice, image, imageMemory, 0);
        }

        static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(instance->logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture image view!");
            }

            return imageView;
        }

        static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                width,
                height,
                1};

            vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);

            EndSingleTimeCommands(commandBuffer);
        }

        static VkCommandBuffer BeginSingleTimeCommands()
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = instance->commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(instance->logicalDevice, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }

        static void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
        {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(instance->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(instance->graphicsQueue);

            vkFreeCommandBuffers(instance->logicalDevice, instance->commandPool, 1, &commandBuffer);
        }

        static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice pdevice)
        {
            SwapChainSupportDetails details;
            VkSurfaceKHR surface = instance->surface;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface, &details.capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &formatCount, nullptr);

            if (formatCount != 0)
            {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0)
            {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice pdevice)
        {
            QueueFamilyIndices indices;
            // Assign index to queue families that could be found
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, queueFamilies.data());

            std::cout << "Queue Family Cnt " << queueFamilyCount << "\n";

            int i = 0;
            for (const auto &queueFamily : queueFamilies)
            {
                if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                    (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
                    indices.graphicsAndComputeFamily = i;

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, i, instance->surface, &presentSupport);

                if (presentSupport)
                    indices.presentFamily = i;

                if (indices.isComplete())
                    break;

                i++;
            }

            return indices;
        }

        int window_width;
        int window_height;

        GLFWwindow *window;
        VkInstance vkinstance;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkQueue graphicsQueue;
        VkQueue computeQueue;
        VkQueue presentQueue;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkFormat depthFormat;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

    private:
        void initWindow();
        void createInstance();
        void createSurface();
        bool isDeviceSuitable(VkPhysicalDevice pdevice);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
    };
}

#endif