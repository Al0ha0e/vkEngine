#ifndef GBUFFER_H
#define GBUFFER_H

#include <render/environment.hpp>

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
        }

        uint32_t width;
        uint32_t height;
        VkImage images[GBUFFER_CNT];
        VkImageView imageViews[GBUFFER_CNT];
        VkSampler sampler;

    private:
        VmaAllocation gbufferImageVmaAllocations[GBUFFER_CNT];

        void dispose()
        {
            vkDestroySampler(globalLogicalDevice, sampler, nullptr);
            RenderEnvironment *env = RenderEnvironment::GetInstance();
            for (int i = 0; i < GBUFFER_CNT; ++i)
            {
                vkDestroyImageView(globalLogicalDevice, imageViews[i], nullptr);
                vmaDestroyImage(env->vmaAllocator, images[i], gbufferImageVmaAllocations[i]);
            }
        }

        void init()
        {
            VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                           VK_IMAGE_USAGE_SAMPLED_BIT |
                                           VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            for (int i = 0; i < GBUFFER_CNT; ++i)
                RenderEnvironment::CreateImage(width, height, gbufferFormats[i],
                                               VK_IMAGE_TILING_OPTIMAL, usageFlags, 1,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &images[i],
                                               &gbufferImageVmaAllocations[i], nullptr);
            RenderEnvironment *instance = RenderEnvironment::GetInstance();
            VkCommandBuffer tmpCmdBuffer = RenderEnvironment::BeginSingleTimeCommands(instance->commandPool);
            for (int i = 0; i < GBUFFER_CNT; ++i)
                RenderEnvironment::MakeLayoutTransition(tmpCmdBuffer, VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, images[i], VK_IMAGE_ASPECT_COLOR_BIT, 1,
                                                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            RenderEnvironment::EndSingleTimeCommands(RenderEnvironment::GetGraphicsQueue(), instance->commandPool, tmpCmdBuffer);
            for (int i = 0; i < GBUFFER_CNT; ++i)
                imageViews[i] = RenderEnvironment::CreateImageView(images[i], gbufferFormats[i], VK_IMAGE_ASPECT_COLOR_BIT);

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
    };
}

#endif