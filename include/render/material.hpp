#ifndef MATERIAL_H
#define MATERIAL_H

#include <render/shader.hpp>
#include <render/descriptor.hpp>
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
        std::shared_ptr<VertFragShader> shader;
        std::vector<std::shared_ptr<Texture2D>> textures;
        std::vector<DescriptorInfo> commonDescriptorInfos;
        std::vector<DescriptorInfo> perUnitDescriptorInfos;
        std::vector<VkBuffer> commonBuffers;

    private:
        void init()
        {
        }
    };
}

#endif