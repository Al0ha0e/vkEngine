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
        std::vector<VkBuffer> buffers;
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;
        std::vector<VkSemaphore> signalSemaphores;
        VkDescriptorSet descriptorSet;

        ComputeTaskInstance(std::vector<VkBuffer> &&buffers,
                            std::vector<VkSemaphore> &&waitSemaphores,
                            std::vector<VkPipelineStageFlags> &&waitStages,
                            std::vector<VkSemaphore> &&signalSemaphores) : buffers(buffers),
                                                                           waitSemaphores(waitSemaphores),
                                                                           waitStages(waitStages),
                                                                           signalSemaphores(signalSemaphores) {}

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
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

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
        std::vector<DescriptorInfo> descriptorInfos;
        std::map<uint64_t, std::unique_ptr<ComputeTaskInstance>> instances;

        ComputeTask() = default;

        ComputeTask(std::shared_ptr<ComputeShader> &shader, std::vector<DescriptorInfo> &&descriptorInfos)
            : shader(shader),
              descriptorInfos(std::forward<std::vector<DescriptorInfo>>(descriptorInfos)),
              descriptorSetInfo(nullptr, 0, 0, 0)
        {
            initDescriptorSetLayout();
            createGraphicsPipeline();
        }

        ~ComputeTask()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyPipeline(logicalDevice, pipeline, nullptr);
            vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        }

        uint64_t AddInstance(std::vector<VkBuffer> &&buffers,
                             std::vector<VkSemaphore> &&waitSemaphores,
                             std::vector<VkPipelineStageFlags> &&waitStages,
                             std::vector<VkSemaphore> &&signalSemaphores)
        {
            uint64_t id = allocator.Alloc();
            ComputeTaskInstance *instance = new ComputeTaskInstance(std::forward<std::vector<VkBuffer>>(buffers),
                                                                    std::forward<std::vector<VkSemaphore>>(waitSemaphores),
                                                                    std::forward<std::vector<VkPipelineStageFlags>>(waitStages),
                                                                    std::forward<std::vector<VkSemaphore>>(signalSemaphores));
            createDescriptorSet(*instance);
            instances[id] = std::move(std::unique_ptr<ComputeTaskInstance>(instance));
            return id;
        }

        void Dispatch(uint64_t id, VkCommandBuffer commandBuffer, glm::ivec3 dim3, VkFence fence)
        {
            instances[id]->Dispatch(commandBuffer, dim3, pipelineLayout, pipeline, fence);
        }

    private:
        DescriptorSetInfo descriptorSetInfo;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        vke_ds::NaiveIDAllocator<uint64_t> allocator;

        void initDescriptorSetLayout()
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            for (auto &dInfo : descriptorInfos)
            {
                descriptorSetInfo.AddCnt(dInfo.bindingInfo.descriptorType);
                bindings.push_back(dInfo.bindingInfo);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = bindings.size();
            layoutInfo.pBindings = bindings.data();

            if (vkCreateDescriptorSetLayout(RenderEnvironment::GetInstance()->logicalDevice,
                                            &layoutInfo,
                                            nullptr,
                                            &(descriptorSetInfo.layout)) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
        }

        void createGraphicsPipeline()
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &(descriptorSetInfo.layout);

            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;

            if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &(pipelineLayout)) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create compute pipeline layout!");
            }

            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.stage = shader->shaderStageInfo;

            if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create compute pipeline!");
            }
        }

        void createDescriptorSet(ComputeTaskInstance &instance)
        {
            instance.descriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(descriptorSetInfo);

            int descriptorCnt = descriptorInfos.size();
            std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorCnt);
            std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorCnt);
            for (int i = 0; i < descriptorCnt; i++)
            {
                DescriptorInfo &info = descriptorInfos[i];

                if (info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                    info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                {
                    InitDescriptorBufferInfo(bufferInfos[i], instance.buffers[i], 0, info.bufferSize);
                    descriptorWrites[i] = ConstructDescriptorSetWrite(instance.descriptorSet, info, bufferInfos.data() + i);
                }
                else if (info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = info.imageView;
                    imageInfo.sampler = info.imageSampler;
                    descriptorWrites[i] = ConstructDescriptorSetWrite(instance.descriptorSet, info, &imageInfo);
                }
            }
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorCnt, descriptorWrites.data(), 0, nullptr);
        }
    };
}

#endif