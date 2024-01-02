#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <render/environment.hpp>
#include <render/renderinfo.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class BaseRenderer : public SubpassBase
    {
    public:
        std::vector<DescriptorInfo> globalDescriptorInfos;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;

        BaseRenderer() {}
        BaseRenderer(int subpassID, VkRenderPass renderPass)
            : SubpassBase(subpassID, renderPass)
        {
            environment = RenderEnvironment::GetInstance();
            createGlobalDescriptorSet();
            createSkyBox();
        }

        void Dispose() override
        {
            vkDestroyPipeline(environment->logicalDevice, renderInfo->pipeline, nullptr);
            vkDestroyPipelineLayout(environment->logicalDevice, renderInfo->pipelineLayout, nullptr);
        }

        void RegisterCamera(VkBuffer buffer) override
        {
            DescriptorInfo &info = globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            InitDescriptorBufferInfo(bufferInfo, buffer, 0, info.bufferSize);
            VkWriteDescriptorSet descriptorWrite = ConstructDescriptorSetWrite(globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }

        void Render(VkCommandBuffer commandBuffer) override;

    private:
        RenderEnvironment *environment;
        RenderInfo *renderInfo;

        void createGlobalDescriptorSet();
        void createSkyBox();
        void createGraphicsPipeline();
    };
}

#endif