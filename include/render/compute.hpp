#ifndef COMPUTE_H
#define COMPUTE_H

#include <render/environment.hpp>
#include <render/shader.hpp>
#include <render/descriptor.hpp>
#include <ds/id_allocator.hpp>

namespace vke_render
{
    class ComputeTask
    {
    public:
        std::shared_ptr<ComputeShader> shader;

        ComputeTask() = default;

        ComputeTask(std::shared_ptr<ComputeShader> &shader) : shader(shader)
        {
            shader->CreatePipeline(pipelineLayout, pipeline);
        }

        ~ComputeTask()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyPipeline(logicalDevice, pipeline, nullptr);
            vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        }

        void Dispatch(VkCommandBuffer commandBuffer, std::vector<VkDescriptorSet> &descriptorSets, glm::ivec3 dim3)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, 0);
            vkCmdDispatch(commandBuffer, dim3.x, dim3.y, dim3.z);
        }

        void CreateDescriptorSets(std::vector<VkDescriptorSet> &descriptorSets)
        {
            ShaderModuleSet &shaderModule = *(shader->shaderModule);

            for (auto &kv : shaderModule.descriptorSetInfoMap)
                descriptorSets.push_back(vke_render::DescriptorSetAllocator::AllocateDescriptorSet(kv.second));
        }

    private:
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
    };
}

#endif