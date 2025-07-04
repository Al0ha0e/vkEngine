#ifndef ENV_H
#define ENV_H

#include <common.hpp>
#include <optional>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <memory>
#include <event.hpp>
#include <vk_mem_alloc.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace vke_render
{
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsAndComputeFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeOnlyFamily;
        std::optional<uint32_t> transferOnlyFamily;
        std::vector<uint32_t> uniqueQueueFamilies;

        void clear()
        {
            graphicsAndComputeFamily.reset();
            presentFamily.reset();
            computeOnlyFamily.reset();
            transferOnlyFamily.reset();
            uniqueQueueFamilies.clear();
        }

        bool isComplete()
        {
            return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
        }

        void getUniqueQueueFamilies()
        {
            std::set<uint32_t> queueFamilySet = {graphicsAndComputeFamily.value(),
                                                 presentFamily.value()};
            if (computeOnlyFamily.has_value())
                queueFamilySet.insert(computeOnlyFamily.value());
            if (transferOnlyFamily.has_value())
                queueFamilySet.insert(transferOnlyFamily.value());
            for (auto &val : queueFamilySet)
                uniqueQueueFamilies.push_back(val);
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct RenderContext
    {
        uint32_t width;
        uint32_t height;
        uint32_t imageCnt;
        VkFormat colorFormat;
        VkFormat depthFormat;
        VkImageLayout outColorLayout;
        VkImage depthImage;
        VkImageView depthImageView;
        std::vector<VkImage> *colorImages;
        std::vector<VkImageView> *colorImageViews;
        vke_common::EventHub<RenderContext> *resizeEventHub;
        std::function<uint32_t(uint32_t)> AcquireNextImage;
        std::function<void(uint32_t, uint32_t)> Present;
    };

    class RenderEnvironment
    {
    private:
        static RenderEnvironment *instance;
        RenderEnvironment() : windowResized(false) {}
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

        static RenderEnvironment *Init(GLFWwindow *window)
        {
            instance = new RenderEnvironment();
            instance->window = window;
            instance->createInstance();
            instance->createSurface();
            instance->pickPhysicalDevice();
            instance->createLogicalDevice();
            instance->createVulkanMemoryAllocator();
            instance->createCommandPool();
            instance->createSyncObjects();
            instance->createSwapChain();
            instance->createImageViews();
            vke_common::EventSystem::AddEventListener(vke_common::EVENT_WINDOW_RESIZE, instance, vke_common::EventCallback(OnWindowResize));
            instance->rootRenderContext = RenderContext{
                instance->swapChainExtent.width,
                instance->swapChainExtent.height,
                instance->imageCnt,
                instance->swapChainImageFormat,
                instance->depthFormat,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                instance->depthImage,
                instance->depthImageView,
                &(instance->swapChainImages),
                &(instance->swapChainImageViews),
                &(instance->resizeEventHub),
                AcquireNextImage,
                Present};
            return instance;
        }

        static void Dispose()
        {
            instance->cleanupSwapChain();
            VkDevice logicalDevice = instance->logicalDevice;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vkDestroySemaphore(logicalDevice, instance->renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(logicalDevice, instance->imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(logicalDevice, instance->inFlightFences[i], nullptr);
            }

            if (instance->computeCommandPool != instance->commandPool)
                vkDestroyCommandPool(logicalDevice, instance->computeCommandPool, nullptr);
            if (instance->transferCommandPool != instance->commandPool)
                vkDestroyCommandPool(logicalDevice, instance->transferCommandPool, nullptr);
            vkDestroyCommandPool(logicalDevice, instance->commandPool, nullptr);
            vmaDestroyAllocator(instance->vmaAllocator);
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

        static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                 VmaAllocationCreateFlags vmaFlags, VmaMemoryUsage vmaUsage,
                                 VkBuffer *buffer, VmaAllocation *vmaAllocation, VmaAllocationInfo *vmaAllocationInfo)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = instance->queueFamilyIndices.uniqueQueueFamilies.size();
            bufferInfo.pQueueFamilyIndices = instance->queueFamilyIndices.uniqueQueueFamilies.data();

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.flags = vmaFlags;
            allocationCreateInfo.usage = vmaUsage;

            vmaCreateBuffer(instance->vmaAllocator, &bufferInfo, &allocationCreateInfo,
                            buffer, vmaAllocation, vmaAllocationInfo);
        }

        static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands(instance->commandPool);

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = srcOffset;
            copyRegion.dstOffset = dstOffset;
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            EndSingleTimeCommands(instance->graphicsQueue, instance->commandPool, commandBuffer);
        }

        static void CreateImage(uint32_t width, uint32_t height, VkFormat format,
                                VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                VkImage *image, VmaAllocation *vmaAllocation, VmaAllocationInfo *vmaAllocationInfo)
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

            VmaAllocationCreateInfo alloationCreateInfo = {};
            alloationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            alloationCreateInfo.preferredFlags = properties;
            vmaCreateImage(instance->vmaAllocator, &imageInfo, &alloationCreateInfo, image, vmaAllocation, vmaAllocationInfo);
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

        static void MakeLayoutTransition(VkCommandBuffer commandBuffer,
                                         VkAccessFlags srcAccessMask,
                                         VkAccessFlags dstAccessMask,
                                         VkImageLayout oldLayout,
                                         VkImageLayout newLayout,
                                         VkImage image,
                                         VkImageAspectFlags aspectMask,
                                         VkPipelineStageFlags sourceStage,
                                         VkPipelineStageFlags destinationStage)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage,
                destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands(instance->transferCommandPool);

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

            EndSingleTimeCommands(instance->transferQueue, instance->transferCommandPool, commandBuffer);
        }

        static VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(instance->logicalDevice, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }

        static void EndSingleTimeCommands(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
        {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            vkFreeCommandBuffers(instance->logicalDevice, commandPool, 1, &commandBuffer);
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

        static void OnWindowResize(void *listener, void *info)
        {
            instance->windowResized = true;
        }

        static uint32_t AcquireNextImage(uint32_t currentFrame)
        {
            vkWaitForFences(instance->logicalDevice, 1, &instance->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(instance->logicalDevice,
                                                    instance->swapChain,
                                                    UINT64_MAX,
                                                    instance->imageAvailableSemaphores[currentFrame],
                                                    VK_NULL_HANDLE,
                                                    &imageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                instance->recreateSwapChain();
                instance->windowResized = false;
                // return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error("failed to acquire swap chain image!");
            }
            vkResetFences(instance->logicalDevice, 1, &instance->inFlightFences[currentFrame]);

            VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &(instance->imageAvailableSemaphores[currentFrame]);
            submitInfo.pWaitDstStageMask = &waitDstStageMask;
            submitInfo.commandBufferCount = 0;
            submitInfo.signalSemaphoreCount = 0;

            if (vkQueueSubmit(RenderEnvironment::GetInstance()->graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            return imageIndex;
        }

        static void Present(uint32_t currentFrame, uint32_t imageIndex)
        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 0;
            submitInfo.commandBufferCount = 0;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &(instance->renderFinishedSemaphores[currentFrame]);

            if (vkQueueSubmit(instance->graphicsQueue, 1, &submitInfo, instance->inFlightFences[currentFrame]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &(instance->renderFinishedSemaphores[currentFrame]);

            VkSwapchainKHR swapChains[] = {instance->swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr; // Optional
            VkResult result = vkQueuePresentKHR(instance->presentQueue, &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || instance->windowResized)
            {
                instance->recreateSwapChain();
                instance->windowResized = false;
            }
            else if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to present swap chain image!");
            }
        }

        uint32_t imageCnt;
        vke_common::EventHub<RenderContext> resizeEventHub;
        GLFWwindow *window;
        VkInstance vkinstance;
        VkSurfaceKHR surface;
        QueueFamilyIndices queueFamilyIndices;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkDevice logicalDevice;
        VkQueue graphicsQueue;
        VkQueue computeQueue;
        VkQueue transferQueue;
        VkQueue presentQueue;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkFormat depthFormat;
        VkCommandPool commandPool;
        VkCommandPool computeCommandPool;
        VkCommandPool transferCommandPool;
        VmaAllocator vmaAllocator;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        VkImage depthImage;
        VmaAllocation depthImageVmaAllocation;
        VkImageView depthImageView;
        RenderContext rootRenderContext;

    private:
        bool windowResized;

        void createInstance();
        void createSurface();
        void findQueueFamilies(VkPhysicalDevice pdevice);
        bool isDeviceSuitable(VkPhysicalDevice pdevice);
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createVulkanMemoryAllocator();
        void createCommandPool();
        void createSyncObjects();
        void createSwapChain();
        void cleanupSwapChain();
        void recreateSwapChain();
        void createImageViews();
    };
}

#endif