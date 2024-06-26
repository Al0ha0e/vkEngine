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

        ~Material() {}

        std::string path;
        std::shared_ptr<VertFragShader> shader;
        std::vector<std::shared_ptr<Texture2D>> textures;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        std::vector<DescriptorInfo> commonDescriptorInfos;
        std::vector<DescriptorInfo> perUnitDescriptorInfos;
        std::vector<VkBuffer> commonBuffers;

        void ApplyToPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
                             VkPipelineShaderStageCreateInfo *&stages) const
        {
            vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
            vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
            vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
            stages = shader->shaderStages.data();
        }

    private:
        void init()
        {
        }
    };
}

#endif