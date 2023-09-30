#ifndef MATERIAL_H
#define MATERIAL_H

#include <render/shader.hpp>

namespace vke_render
{
    class Material
    {
    public:
        Material() = default;

        Shader *shader;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        int uniformOffset;

        void ApplyToPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
                             VkPipelineShaderStageCreateInfo *&stages)
        {
            vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
            vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
            vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
            stages = shader->shaderStages.data();
        }
    };
}

#endif