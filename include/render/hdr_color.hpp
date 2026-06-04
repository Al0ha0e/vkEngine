#ifndef HDR_COLOR_H
#define HDR_COLOR_H

#include <render/frame_graph.hpp>

namespace vke_render
{
    class HDRColorManager
    {
    public:
        static constexpr VkFormat HDR_COLOR_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkSampler sampler;

        HDRColorManager(RenderContext *ctx)
            : context(ctx), sampler(VK_NULL_HANDLE), resourceID(0)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                images[i] = VK_NULL_HANDLE;
                imageViews[i] = VK_NULL_HANDLE;
            }
            createImages();
            createSampler();
        }

        ~HDRColorManager()
        {
            cleanupImageViews();
            cleanupImages();
            if (sampler != VK_NULL_HANDLE)
                vkDestroySampler(globalLogicalDevice, sampler, nullptr);
        }

        HDRColorManager(const HDRColorManager &) = delete;
        HDRColorManager &operator=(const HDRColorManager &) = delete;

        void ConstructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 CurrentResourceNodeIDMaps &currentResourceNodeID)
        {
            resourceID = frameGraph.AddTransientImageResource("hdrColor", images, VK_IMAGE_ASPECT_COLOR_BIT);
            blackboard["hdrColor"] = resourceID;
        }

        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx)
        {
            context = ctx;
            cleanupImageViews();
            cleanupImages();
            createImages();

            ImageResource *resource = (ImageResource *)frameGraph.transientResources[resourceID].get();
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                resource->images[i] = images[i];
        }

        void CreateImageView(uint32_t currentFrame)
        {
            if (imageViews[currentFrame] != VK_NULL_HANDLE)
                vkDestroyImageView(globalLogicalDevice, imageViews[currentFrame], nullptr);
            imageViews[currentFrame] = RenderEnvironment::CreateImageView(images[currentFrame], HDR_COLOR_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        VkImageView GetImageView(uint32_t currentFrame) const { return imageViews[currentFrame]; }

    private:
        RenderContext *context;
        VkImage images[MAX_FRAMES_IN_FLIGHT];
        VkImageView imageViews[MAX_FRAMES_IN_FLIGHT];

        vke_ds::id32_t resourceID;

        void createImages()
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                RenderEnvironment::CreateImageWithoutMemory(context->width, context->height,
                                                            HDR_COLOR_FORMAT, VK_IMAGE_TILING_OPTIMAL,
                                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                VK_IMAGE_USAGE_SAMPLED_BIT |
                                                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                            1, &images[i]);
        }

        void cleanupImages()
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                if (images[i] != VK_NULL_HANDLE)
                {
                    vkDestroyImage(globalLogicalDevice, images[i], nullptr);
                    images[i] = VK_NULL_HANDLE;
                }
        }

        void cleanupImageViews()
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                if (imageViews[i] != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(globalLogicalDevice, imageViews[i], nullptr);
                    imageViews[i] = VK_NULL_HANDLE;
                }
        }

        void createSampler()
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;

            VKE_VK_CHECK(vkCreateSampler(globalLogicalDevice, &samplerInfo, nullptr, &sampler), "failed to create hdr color sampler!")
        }
    };
}

#endif
