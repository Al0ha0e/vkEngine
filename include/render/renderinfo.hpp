#ifndef RENDER_INFO_H
#define RENDER_INFO_H

#include <render/environment.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/buffer.hpp>
#include <ds/id_allocator.hpp>

namespace vke_render
{
    struct CameraInfo
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 viewPos;

        CameraInfo() = default;

        CameraInfo(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos)
            : view(view), projection(projection), viewPos(viewPos, 1.0) {}
    };

    struct RenderUnit
    {
        Mesh *mesh;
        VkDescriptorSet descriptorSet;

        RenderUnit() = default;

        RenderUnit(Mesh *msh) : mesh(msh), descriptorSet(nullptr) {}

        RenderUnit(
            Mesh *msh,
            DescriptorSetInfo &descriptorSetInfo,
            std::vector<DescriptorInfo> &descriptorInfos,
            std::vector<HostCoherentBuffer> &buffers) : mesh(msh)
        {
            descriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(descriptorSetInfo);
            int descriptorCnt = descriptorInfos.size();
            std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorCnt);
            std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorCnt);
            for (int i = 0; i < descriptorCnt; i++)
            {
                DescriptorInfo &info = descriptorInfos[i];
                VkDescriptorBufferInfo &bufferInfo = bufferInfos[i];
                bufferInfo.buffer = buffers[i].buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = info.bufferSize;

                descriptorWrites[i] = ConstructDescriptorSetWrite(descriptorSet, info, bufferInfos.data() + i);
            }
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorCnt, descriptorWrites.data(), 0, nullptr);
        }

        void Render(VkCommandBuffer &commandBuffer, VkPipelineLayout &pipelineLayout, int descriptorSetOffset)
        {
            if (descriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    descriptorSetOffset,
                    1,
                    &descriptorSet,
                    0, nullptr);

            mesh->Render(commandBuffer);
        }
    };

    class RenderInfo
    {
    public:
        Material *material;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        DescriptorSetInfo commonDescriptorSetInfo;
        VkDescriptorSet commonDescriptorSet;
        DescriptorSetInfo perUnitDescriptorSetInfo;
        bool hasCommonDescriptorSet;
        bool hasPerUnitDescriptorSet;
        std::map<uint64_t, RenderUnit> units;

        RenderInfo() = default;

        RenderInfo(Material *mat)
            : material(mat),
              commonDescriptorSet(nullptr)
        {
            hasCommonDescriptorSet = mat->commonDescriptorInfos.size() > 0;
            hasPerUnitDescriptorSet = mat->perUnitDescriptorInfos.size() > 0;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            if (hasCommonDescriptorSet)
            {
                std::vector<DescriptorInfo> &commonDescriptorInfos = mat->commonDescriptorInfos;
                commonDescriptorSetInfo.uniformDescriptorCnt = 0;
                commonDescriptorSetInfo.combinedImageSamplerCnt = 0;
                std::vector<VkDescriptorSetLayoutBinding> commonBindings;
                for (auto &dInfo : commonDescriptorInfos)
                {
                    commonDescriptorSetInfo.AddCnt(dInfo.bindingInfo.descriptorType);
                    commonBindings.push_back(dInfo.bindingInfo);
                }

                layoutInfo.bindingCount = commonBindings.size();
                layoutInfo.pBindings = commonBindings.data();

                if (vkCreateDescriptorSetLayout(
                        RenderEnvironment::GetInstance()->logicalDevice,
                        &layoutInfo,
                        nullptr,
                        &(commonDescriptorSetInfo.layout)) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout!");
                }
                commonDescriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(commonDescriptorSetInfo);

                int descriptorCnt = commonDescriptorInfos.size();
                // std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorCnt);
                std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorCnt);
                for (int i = 0; i < descriptorCnt; i++)
                {
                    DescriptorInfo &info = commonDescriptorInfos[i];

                    if (info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    {
                        // VkDescriptorBufferInfo &bufferInfo = bufferInfos[i];
                        VkDescriptorBufferInfo bufferInfo = {};
                        bufferInfo.buffer = material->commonBuffers[i];
                        bufferInfo.offset = 0;
                        bufferInfo.range = info.bufferSize;
                        descriptorWrites[i] = ConstructDescriptorSetWrite(commonDescriptorSet, info, &bufferInfo);
                    }
                    else if (info.bindingInfo.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    {
                        VkDescriptorImageInfo imageInfo{};
                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        imageInfo.imageView = info.imageView;
                        imageInfo.sampler = info.imageSampler;
                        descriptorWrites[i] = ConstructDescriptorSetWrite(commonDescriptorSet, info, &imageInfo);
                    }
                }
                vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorCnt, descriptorWrites.data(), 0, nullptr);
            }

            if (hasPerUnitDescriptorSet)
            {
                perUnitDescriptorSetInfo.uniformDescriptorCnt = 0;
                std::vector<VkDescriptorSetLayoutBinding> perUnitBindings;
                for (auto &dInfo : mat->perUnitDescriptorInfos)
                {
                    perUnitDescriptorSetInfo.AddCnt(dInfo.bindingInfo.descriptorType);
                    perUnitBindings.push_back(dInfo.bindingInfo);
                }

                layoutInfo.bindingCount = perUnitBindings.size();
                layoutInfo.pBindings = perUnitBindings.data();

                if (vkCreateDescriptorSetLayout(
                        RenderEnvironment::GetInstance()->logicalDevice,
                        &layoutInfo,
                        nullptr,
                        &(perUnitDescriptorSetInfo.layout)) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout!");
                }
            }
        }

        ~RenderInfo()
        {
            VkDevice logicalDevice = RenderEnvironment::GetInstance()->logicalDevice;
            if (hasCommonDescriptorSet)
                vkDestroyDescriptorSetLayout(logicalDevice, commonDescriptorSetInfo.layout, nullptr);
            if (hasPerUnitDescriptorSet)
                vkDestroyDescriptorSetLayout(logicalDevice, perUnitDescriptorSetInfo.layout, nullptr);
        }

        void ApplyToPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputInfo,
                             VkPipelineShaderStageCreateInfo *&stages,
                             std::vector<VkDescriptorSetLayout> &globalDescriptorSetLayouts)
        {
            material->ApplyToPipeline(vertexInputInfo, stages);

            if (hasCommonDescriptorSet)
                globalDescriptorSetLayouts.push_back(commonDescriptorSetInfo.layout);
            if (hasPerUnitDescriptorSet)
                globalDescriptorSetLayouts.push_back(perUnitDescriptorSetInfo.layout);
        }

        uint64_t AddUnit(Mesh *mesh, std::vector<HostCoherentBuffer> &buffers)
        // uint64_t AddUnit(Mesh *mesh, std::vector<VkBuffer> &buffers)
        {
            uint64_t id = allocator.Alloc();
            units[id] = hasPerUnitDescriptorSet ? RenderUnit(mesh, perUnitDescriptorSetInfo, material->perUnitDescriptorInfos, buffers)
                                                : RenderUnit(mesh);
            return id;
        }

        void Render(VkCommandBuffer &commandBuffer, VkDescriptorSet *globalDescriptorSet)
        {
            int setcnt = 0;
            if (*globalDescriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    setcnt++,
                    1,
                    globalDescriptorSet,
                    0, nullptr);

            if (hasCommonDescriptorSet)
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    setcnt++,
                    1,
                    &commonDescriptorSet,
                    0, nullptr);
            for (auto &unit : units)
                unit.second.Render(commandBuffer, pipelineLayout, setcnt);
        }

    private:
        vke_ds::NaiveIDAllocator<uint64_t> allocator;
    };
}

#endif