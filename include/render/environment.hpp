#ifndef ENV_H
#define ENV_H

#include <common.hpp>
#include <optional>
#include <vector>

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

        void Init(int width, int height)
        {
            window_width = width;
            window_height = height;
            initWindow();
            createInstance();
            createSurface();
            pickPhysicalDevice();
            createLogicalDevice();
            createSwapChain();
            createImageViews();
            createCommandPool();
            createCommandBuffers();
            createSyncObjects();
        }

        void Dispose()
        {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
            }
            vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
            for (auto imageView : swapChainImageViews)
                vkDestroyImageView(logicalDevice, imageView, nullptr);
            vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
            vkDestroyDevice(logicalDevice, nullptr);
            vkDestroySurfaceKHR(vkinstance, surface, nullptr);
            vkDestroyInstance(vkinstance, nullptr);
            glfwTerminate();
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