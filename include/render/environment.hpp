#ifndef ENV_H
#define ENV_H

#include <common.hpp>
#include <optional>
#include <vector>
#include <iostream>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace vke_render
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const int MAX_FRAMES_IN_FLIGHT = 2;

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
                instance = new RenderEnvironment();
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
            instance->createSwapChain();
            instance->createImageViews();
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
            for (auto imageView : instance->swapChainImageViews)
                vkDestroyImageView(logicalDevice, imageView, nullptr);
            vkDestroySwapchainKHR(logicalDevice, instance->swapChain, nullptr);
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
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(instance->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(instance->graphicsQueue);

            vkFreeCommandBuffers(instance->logicalDevice, instance->commandPool, 1, &commandBuffer);
        }

        int window_width;
        int window_height;

        GLFWwindow *window;
        VkInstance vkinstance;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

    private:
        void initWindow();
        void createInstance();
        void createSurface();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pdevice);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pdevice);
        bool isDeviceSuitable(VkPhysicalDevice pdevice);
        void pickPhysicalDevice();
        void createLogicalDevice();
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        void createSwapChain();
        void createImageViews();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
    };
}

#endif