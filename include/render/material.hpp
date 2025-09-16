#ifndef MATERIAL_H
#define MATERIAL_H

#include <render/shader.hpp>
#include <render/texture.hpp>

namespace vke_render
{
    class Material
    {
    public:
        Material() = default;

        Material(vke_common::AssetHandle hdl) : handle(hdl) {}

        ~Material() {}

        vke_common::AssetHandle handle;
        std::shared_ptr<ShaderModuleSet> shader;
        std::vector<std::shared_ptr<Texture2D>> textures;

        void UpdateDescriptorSet(VkDescriptorSet descriptorSet, uint32_t st = 0)
        {
            std::vector<VkWriteDescriptorSet> writes(textures.size());
            std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
            for (int i = 0; i < textures.size(); i++)
            {
                std::shared_ptr<Texture2D> &texture = textures[i];
                VkDescriptorImageInfo &imageInfo = imageInfos[i];
                imageInfo.sampler = texture->textureSampler;
                imageInfo.imageView = texture->textureImageView;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                vke_render::ConstructDescriptorSetWrite(writes[i], descriptorSet, st + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
            }
            vkUpdateDescriptorSets(globalLogicalDevice, writes.size(), writes.data(), 0, nullptr);
        }
    };
}

#endif