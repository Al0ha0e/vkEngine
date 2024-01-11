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

        BaseRenderer() : globalDescriptorSetInfo(nullptr, 0, 0, 0) {}

        ~BaseRenderer() {}

        void Init(int subpassID, VkRenderPass renderPass) override
        {
            SubpassBase::Init(subpassID, renderPass);
            environment = RenderEnvironment::GetInstance();
            createGlobalDescriptorSet();
            createSkyBox();
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
        std::unique_ptr<RenderInfo> renderInfo;

        void createGlobalDescriptorSet();
        void createSkyBox();
        void createGraphicsPipeline();
    };
}

#endif