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
            : context(ctx), sampler(VK_NULL_HANDLE), latestImageIndex(0)
        {
            for (uint32_t imageIndex = 0; imageIndex < IMAGE_COUNT; ++imageIndex)
            {
                resourceIDs[imageIndex] = 0;
                resourceNodeIDs[imageIndex] = 0;
                for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
                {
                    images[imageIndex][frame] = VK_NULL_HANDLE;
                    imageViews[imageIndex][frame] = VK_NULL_HANDLE;
                }
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
                                 ResourceNodeIDMap &currentResourceNodeID)
        {
            latestImageIndex = 0;
            for (uint32_t imageIndex = 0; imageIndex < IMAGE_COUNT; ++imageIndex)
            {
                resourceNodeIDs[imageIndex] = 0;
                resourceIDs[imageIndex] = frameGraph.AddTransientImageResource(
                    imageIndex == 0 ? "hdrColor0" : "hdrColor1", images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT);
            }
            frameGraph.AddTransientReadyCallback([this](uint32_t currentFrame)
                                                 { CreateImageViews(currentFrame); });
        }

        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx)
        {
            context = ctx;
            cleanupImageViews();
            cleanupImages();
            createImages();

            for (uint32_t imageIndex = 0; imageIndex < IMAGE_COUNT; ++imageIndex)
            {
                ImageResource *resource = static_cast<ImageResource *>(frameGraph.resources[resourceIDs[imageIndex]].get());
                for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
                    resource->images[frame] = images[imageIndex][frame];
            }
        }

        void CreateImageViews(uint32_t currentFrame)
        {
            for (uint32_t imageIndex = 0; imageIndex < IMAGE_COUNT; ++imageIndex)
            {
                if (imageViews[imageIndex][currentFrame] != VK_NULL_HANDLE)
                    vkDestroyImageView(globalLogicalDevice, imageViews[imageIndex][currentFrame], nullptr);
                imageViews[imageIndex][currentFrame] = RenderEnvironment::CreateImageView(
                    images[imageIndex][currentFrame], HDR_COLOR_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }

        static constexpr uint32_t IMAGE_COUNT = 2;
        uint32_t GetLatestImageIndex() const { return latestImageIndex; }
        uint32_t GetAlternateImageIndex() const { return 1u - latestImageIndex; }
        vke_ds::id32_t GetResourceID(uint32_t imageIndex) const { return resourceIDs[imageIndex]; }
        vke_ds::id32_t GetResourceNodeID(uint32_t imageIndex) const { return resourceNodeIDs[imageIndex]; }
        VkImageView GetImageView(uint32_t imageIndex, uint32_t currentFrame) const { return imageViews[imageIndex][currentFrame]; }
        VkImageView GetLatestImageView(uint32_t currentFrame) const { return GetImageView(latestImageIndex, currentFrame); }

        void UpdateResourceNode(uint32_t imageIndex, vke_ds::id32_t resourceNodeID)
        {
            resourceNodeIDs[imageIndex] = resourceNodeID;
            latestImageIndex = imageIndex;
        }

    private:
        RenderContext *context;
        VkImage images[IMAGE_COUNT][MAX_FRAMES_IN_FLIGHT];
        VkImageView imageViews[IMAGE_COUNT][MAX_FRAMES_IN_FLIGHT];
        vke_ds::id32_t resourceIDs[IMAGE_COUNT];
        vke_ds::id32_t resourceNodeIDs[IMAGE_COUNT];
        uint32_t latestImageIndex;

        void createImages()
        {
            for (uint32_t imageIndex = 0; imageIndex < IMAGE_COUNT; ++imageIndex)
                for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
                    RenderEnvironment::CreateImageWithoutMemory(context->width, context->height,
                                                            HDR_COLOR_FORMAT, VK_IMAGE_TILING_OPTIMAL,
                                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                VK_IMAGE_USAGE_SAMPLED_BIT |
                                                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                            1, &images[imageIndex][frame]);
        }

        void cleanupImages()
        {
            for (auto &imageSet : images)
                for (VkImage &image : imageSet)
                    if (image != VK_NULL_HANDLE)
                    {
                        vkDestroyImage(globalLogicalDevice, image, nullptr);
                        image = VK_NULL_HANDLE;
                    }
        }

        void cleanupImageViews()
        {
            for (auto &imageViewSet : imageViews)
                for (VkImageView &imageView : imageViewSet)
                    if (imageView != VK_NULL_HANDLE)
                    {
                        vkDestroyImageView(globalLogicalDevice, imageView, nullptr);
                        imageView = VK_NULL_HANDLE;
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
