#ifndef TEXTURE_H
#define TEXTURE_H

#include <common.hpp>
#include <render/environment.hpp>

namespace vke_render
{
    class Texture2D
    {
    public:
        vke_common::AssetHandle handle;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        Texture2D(const vke_common::AssetHandle hdl, void *pixels, int texWidth, int texHeight) : handle(hdl)
        {
            VkDeviceSize imageSize = texWidth * texHeight * 4;
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            RenderEnvironment::CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);

            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;

            void *data;
            vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(logicalDevice, stagingBufferMemory);

            RenderEnvironment::CreateImage(
                texWidth,
                texHeight,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage,
                textureImageMemory);

            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            RenderEnvironment::CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
            vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

            textureImageView = RenderEnvironment::CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
            createTextureSampler();
        }

        ~Texture2D()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroySampler(logicalDevice, textureSampler, nullptr);
            vkDestroyImageView(logicalDevice, textureImageView, nullptr);
            vkDestroyImage(logicalDevice, textureImage, nullptr);
            vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
        }

    private:
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            RenderEnvironment *instance = RenderEnvironment::GetInstance();
            VkCommandBuffer commandBuffer = RenderEnvironment::BeginSingleTimeCommands(instance->commandPool);

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                RenderEnvironment::MakeLayoutTransition(commandBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                        oldLayout, newLayout,
                                                        image, VK_IMAGE_ASPECT_COLOR_BIT,
                                                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                RenderEnvironment::MakeLayoutTransition(commandBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                                        oldLayout, newLayout,
                                                        image, VK_IMAGE_ASPECT_COLOR_BIT,
                                                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            else
                throw std::invalid_argument("unsupported layout transition!");

            RenderEnvironment::EndSingleTimeCommands(instance->graphicsQueue, instance->commandPool, commandBuffer);
        }

        void createTextureSampler()
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = RenderEnvironment::GetInstance()->physicalDeviceProperties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(RenderEnvironment::GetInstance()->logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture sampler!");
            }
        }
    };
}

#endif