#ifndef GBUFFER_H
#define GBUFFER_H

#include <render/environment.hpp>
#include <render/frame_graph.hpp>

namespace vke_render
{
    const uint32_t GBUFFER_CNT = 4;
    const VkFormat gbufferFormats[GBUFFER_CNT] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A2R10G10B10_UNORM_PACK32,
                                                  VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32_SFLOAT};

    class GBuffer
    {
    private:
        static GBuffer *instance;
        GBuffer() {}
        GBuffer(uint32_t w, uint32_t h) : width(w), height(h) {}
        ~GBuffer() {}
        GBuffer(const GBuffer &);
        GBuffer &operator=(const GBuffer);

    public:
        static GBuffer *GetInstance()
        {
            return instance;
        }

        static GBuffer *Init(uint32_t w, uint32_t h)
        {
            if (instance != nullptr)
                return instance;
            instance = new GBuffer(w, h);
            instance->init();
            return instance;
        }

        static void Dispose()
        {
            instance->dispose();
            delete instance;
            instance = nullptr;
        }

        void RegisterFrameGraphResources(FrameGraph &frameGraph)
        {
            for (int i = 0; i < GBUFFER_CNT; ++i)
            {
                resourceIDs[i] = frameGraph.AddTransientImageResource("gbuffer" + std::to_string(i), images[i], VK_IMAGE_ASPECT_COLOR_BIT);
                resourceNodeIDs[i] = frameGraph.AllocResourceNode("oriGBuffer" + std::to_string(i), true, resourceIDs[i]);
            }
        }

        vke_ds::id32_t GetResourceID(uint32_t index) const
        {
            return resourceIDs[index];
        }

        vke_ds::id32_t GetResourceNodeID(uint32_t index) const
        {
            return resourceNodeIDs[index];
        }

        void CreateImageViews(uint32_t currentFrame)
        {
            for (int i = 0; i < GBUFFER_CNT; ++i)
            {
                if (imageViews[i][currentFrame] != VK_NULL_HANDLE)
                    vkDestroyImageView(globalLogicalDevice, imageViews[i][currentFrame], nullptr);
                imageViews[i][currentFrame] = RenderEnvironment::CreateImageView(images[i][currentFrame], gbufferFormats[i], VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }

        static void Recreate(uint32_t w, uint32_t h)
        {
            instance->width = w;
            instance->height = h;
            instance->cleanupImageViews();
            instance->cleanupImages();
            instance->createImages();
        }

        uint32_t width;
        uint32_t height;
        VkImage images[GBUFFER_CNT][MAX_FRAMES_IN_FLIGHT];
        VkImageView imageViews[GBUFFER_CNT][MAX_FRAMES_IN_FLIGHT];
        VkSampler sampler;
        vke_ds::id32_t resourceIDs[GBUFFER_CNT];
        vke_ds::id32_t resourceNodeIDs[GBUFFER_CNT];

    private:
        void dispose()
        {
            vkDestroySampler(globalLogicalDevice, sampler, nullptr);
            cleanupImageViews();
            cleanupImages();
        }

        void init()
        {
            for (int i = 0; i < GBUFFER_CNT; ++i)
            {
                resourceIDs[i] = 0;
                resourceNodeIDs[i] = 0;
                for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                    imageViews[i][j] = VK_NULL_HANDLE;
            }

            createImages();

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;

            vkCreateSampler(globalLogicalDevice, &samplerInfo, nullptr, &sampler);
        }

        void createImages()
        {
            VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                           VK_IMAGE_USAGE_SAMPLED_BIT |
                                           VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            for (int i = 0; i < GBUFFER_CNT; ++i)
                for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                    RenderEnvironment::CreateImageWithoutMemory(width, height, gbufferFormats[i],
                                                                VK_IMAGE_TILING_OPTIMAL, usageFlags, 1,
                                                                &images[i][j]);
        }

        void cleanupImages()
        {
            for (int i = 0; i < GBUFFER_CNT; ++i)
                for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                    vkDestroyImage(globalLogicalDevice, images[i][j], nullptr);
        }

        void cleanupImageViews()
        {
            for (int i = 0; i < GBUFFER_CNT; ++i)
                for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                    if (imageViews[i][j] != VK_NULL_HANDLE)
                    {
                        vkDestroyImageView(globalLogicalDevice, imageViews[i][j], nullptr);
                        imageViews[i][j] = VK_NULL_HANDLE;
                    }
        }
    };
}

#endif