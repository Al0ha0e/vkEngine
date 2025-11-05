#ifndef MATERIAL_H
#define MATERIAL_H

#include <render/shader.hpp>
#include <render/texture.hpp>

namespace vke_render
{
    struct TextureBindingInfo
    {
        uint32_t binding;
        uint32_t offset;
        uint32_t cnt;

        TextureBindingInfo() {}
        TextureBindingInfo(uint32_t binding, uint32_t offset, uint32_t cnt)
            : binding(binding), offset(offset), cnt(cnt) {}
    };

    class Material
    {
    public:
        Material() = default;

        Material(vke_common::AssetHandle hdl) : handle(hdl) {}

        ~Material() {}

        vke_common::AssetHandle handle;
        std::shared_ptr<ShaderModuleSet> shader;
        std::vector<std::shared_ptr<Texture2D>> textures;
        std::shared_ptr<std::vector<TextureBindingInfo>> textureBindingInfos;

        void UpdateDescriptorSet(VkDescriptorSet descriptorSet)
        {
            auto &bindingInfos = *textureBindingInfos;
            std::vector<VkWriteDescriptorSet> writes(bindingInfos.size());
            std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
            int texOffset = 0, writeOffset = 0;
            for (auto &bindingInfo : bindingInfos)
            {
                VkDescriptorImageInfo *infop = imageInfos.data() + texOffset;
                for (int j = 0; j < bindingInfo.cnt; ++j, ++texOffset)
                {
                    std::shared_ptr<Texture2D> &texture = textures[texOffset];
                    imageInfos[texOffset] = {texture->textureSampler, texture->textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                }
                vke_render::ConstructDescriptorSetWrite(writes[writeOffset++], descriptorSet, bindingInfo.binding,
                                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, infop, bindingInfo.offset, bindingInfo.cnt);
            }
            vkUpdateDescriptorSets(globalLogicalDevice, writes.size(), writes.data(), 0, nullptr);
        }
    };
}

#endif