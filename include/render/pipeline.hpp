#ifndef PIPELINE_H
#define PIPELINE_H

#include <render/shader.hpp>
#include <glm/vec3.hpp>

namespace vke_render
{
    class GraphicsPipeline
    {
    public:
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        GraphicsPipeline() = default;

        GraphicsPipeline(std::shared_ptr<ShaderModuleSet> &shader,
                         std::vector<uint32_t> &vertexAttributeSizes,
                         VkVertexInputRate vertexInputRate,
                         VkGraphicsPipelineCreateInfo &pipelineInfo) : shader(shader)
        {
            createPipeline(vertexAttributeSizes, vertexInputRate, pipelineInfo);
        }

        ~GraphicsPipeline()
        {
            if (pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(globalLogicalDevice, pipeline, nullptr);
                vkDestroyPipelineLayout(globalLogicalDevice, pipelineLayout, nullptr);
            }
        }

        void Bind(VkCommandBuffer commandBuffer)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        }

    private:
        std::shared_ptr<ShaderModuleSet> shader;
        void createPipeline(std::vector<uint32_t> &vertexAttributeSizes,
                            VkVertexInputRate vertexInputRate,
                            VkGraphicsPipelineCreateInfo &pipelineInfo);
    };

    class ComputePipeline
    {
    public:
        std::shared_ptr<ShaderModuleSet> shader;

        ComputePipeline() = default;

        ComputePipeline(std::shared_ptr<ShaderModuleSet> &shader) : shader(shader)
        {
            createPipeline();
        }

        ComputePipeline(std::shared_ptr<ShaderModuleSet> &&shader) : shader(std::move(shader))
        {
            createPipeline();
        }

        ~ComputePipeline()
        {
            vkDestroyPipeline(globalLogicalDevice, pipeline, nullptr);
            vkDestroyPipelineLayout(globalLogicalDevice, pipelineLayout, nullptr);
        }

        void Dispatch(VkCommandBuffer commandBuffer, const std::vector<VkDescriptorSet> &descriptorSets, glm::ivec3 dim3)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, 0);
            vkCmdDispatch(commandBuffer, dim3.x, dim3.y, dim3.z);
        }

        void Dispatch(VkCommandBuffer commandBuffer, const std::vector<VkDescriptorSet> &descriptorSets, void *constants, glm::ivec3 dim3)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, 0);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, shader->pushConstantRangeMap.begin()->second.size, constants);
            vkCmdDispatch(commandBuffer, dim3.x, dim3.y, dim3.z);
        }

        void CreateDescriptorSets(std::vector<VkDescriptorSet> &descriptorSets)
        {
            shader->CreateDescriptorSets(descriptorSets);
        }

    private:
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        void createPipeline();
    };
}

#endif