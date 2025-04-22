#ifndef COMPUTE_H
#define COMPUTE_H

#include <render/environment.hpp>
#include <render/shader.hpp>
#include <render/descriptor.hpp>
#include <ds/id_allocator.hpp>
#include <engine.hpp>

namespace vke_render
{
    class ComputeTaskInstance
    {
    public:
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<VkDescriptorSet> descriptorSets;

        ComputeTaskInstance(std::vector<VkSemaphore> &&waitSemaphores,
                            std::vector<VkPipelineStageFlags> &&waitStages,
                            std::vector<VkSemaphore> &&signalSemaphores) : waitSemaphores(waitSemaphores),
                                                                           waitStages(waitStages),
                                                                           signalSemaphores(signalSemaphores) {}
        ~ComputeTaskInstance()
        {
            // TODO Free descriptor set
        }

        void UpdateBindings(std::vector<VkWriteDescriptorSet> &descriptorWrites)
        {
            for (auto &descriptorWrite : descriptorWrites)
                descriptorWrite.dstSet = descriptorSets[(int)descriptorWrite.dstSet];
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }

        void Dispatch(
            VkCommandBuffer commandBuffer,
            glm::ivec3 dim3,
            VkPipelineLayout pipelineLayout,
            VkPipeline pipeline,
            VkFence fence)
        {
            vkResetCommandBuffer(commandBuffer, 0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, 0);

            vkCmdDispatch(commandBuffer, dim3.x, dim3.y, dim3.z);

            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = waitSemaphores.size();
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages.data();
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            submitInfo.signalSemaphoreCount = signalSemaphores.size();
            submitInfo.pSignalSemaphores = signalSemaphores.data();

            if (vkQueueSubmit(
                    RenderEnvironment::GetInstance()->computeQueue,
                    1,
                    &submitInfo,
                    fence) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to submit compute command buffer!");
            };

            if (fence)
            {
                VkDevice logicalDevice = vke_common::Engine::GetInstance()->environment->logicalDevice;
                vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, uint64_t(-1));
                vkResetFences(logicalDevice, 1, &fence);
            }
        }
    };

    class ComputeTask
    {
    public:
        std::shared_ptr<ComputeShader> shader;
        std::map<vke_ds::id64_t, std::unique_ptr<ComputeTaskInstance>> instances;

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

        vke_ds::id64_t AddInstance(std::vector<VkSemaphore> &&waitSemaphores,
                                   std::vector<VkPipelineStageFlags> &&waitStages,
                                   std::vector<VkSemaphore> &&signalSemaphores)
        {
            vke_ds::id64_t id = allocator.Alloc();
            ComputeTaskInstance *instance = new ComputeTaskInstance(std::move(waitSemaphores),
                                                                    std::move(waitStages),
                                                                    std::move(signalSemaphores));
            createDescriptorSet(*instance);
            instances[id] = std::move(std::unique_ptr<ComputeTaskInstance>(instance));
            return id;
        }
        void UpdateBindings(vke_ds::id64_t id, std::vector<VkWriteDescriptorSet> &descriptorWrites)
        {
            instances[id]->UpdateBindings(descriptorWrites);
        }

        void Dispatch(vke_ds::id64_t id, VkCommandBuffer commandBuffer, glm::ivec3 dim3, VkFence fence)
        {
            instances[id]->Dispatch(commandBuffer, dim3, pipelineLayout, pipeline, fence);
        }

    private:
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> allocator;

        void createDescriptorSet(ComputeTaskInstance &instance)
        {
            ShaderModuleSet &shaderModule = *(shader->shaderModule);

            for (auto &kv : shaderModule.descriptorSetInfoMap)
                instance.descriptorSets.push_back(vke_render::DescriptorSetAllocator::AllocateDescriptorSet(kv.second));
        }
    };
}

#endif