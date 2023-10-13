#ifndef COMPUTE_H
#define COMPUTE_H

#include <render/environment.hpp>
#include <render/shader.hpp>
#include <render/descriptor.hpp>

namespace vke_render
{
    class ComputeTask
    {
    public:
        ComputeShader *shader;
        std::vector<DescriptorInfo> descriptorInfos;
        std::vector<VkBuffer> buffers;

        ComputeTask() = default;

        ComputeTask(
            ComputeShader *shader,
            std::vector<DescriptorInfo> &&descriptorInfos,
            std::vector<VkBuffer> &&buffers)
            : shader(shader),
              descriptorInfos(descriptorInfos),
              descriptorSetInfo(nullptr, 0, 0, 0),
              buffers(buffers)
        {
            createDescriptorSet();
            createGraphicsPipeline();
        }

        void Dispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
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

            vkCmdDispatch(commandBuffer, x, y, z);

            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            // TODO

            if (vkQueueSubmit(RenderEnvironment::GetInstance()->computeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to submit compute command buffer!");
            };
        }

        ~ComputeTask()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            vkDestroyPipeline(logicalDevice, pipeline, nullptr);
            vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        }

    private:
        DescriptorSetInfo descriptorSetInfo;
        VkDescriptorSet descriptorSet;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        void createDescriptorSet()
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
            descriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(descriptorSetInfo);

            int descriptorCnt = descriptorInfos.size();
            std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorCnt);
            for (int i = 0; i < descriptorCnt; i++)
            {
                DescriptorInfo &info = descriptorInfos[i];

                if (info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                    info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                {
                    // VkDescriptorBufferInfo &bufferInfo = bufferInfos[i];
                    VkDescriptorBufferInfo bufferInfo = {};
                    bufferInfo.buffer = buffers[i];
                    bufferInfo.offset = 0;
                    bufferInfo.range = info.bufferSize;
                    descriptorWrites[i] = ConstructDescriptorSetWrite(descriptorSet, info, &bufferInfo);
                }
                else if (info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = info.imageView;
                    imageInfo.sampler = info.imageSampler;
                    descriptorWrites[i] = ConstructDescriptorSetWrite(descriptorSet, info, &imageInfo);
                }
            }
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorCnt, descriptorWrites.data(), 0, nullptr);
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
    };
}

#endif