#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <render/environment.hpp>
#include <render/renderinfo.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>

namespace vke_render
{
    class BaseRenderer
    {
    private:
        static BaseRenderer *instance;
        BaseRenderer() = default;
        ~BaseRenderer() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (BaseRenderer::instance != nullptr)
                    delete BaseRenderer::instance;
            }
        };
        static Deletor deletor;

    public:
        int subpassID;
        VkRenderPass renderPass;

        std::vector<DescriptorInfo> globalDescriptorInfos;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;

        static BaseRenderer *GetInstance()
        {
            if (instance == nullptr)
                instance = new BaseRenderer();
            return instance;
        }

        static BaseRenderer *Init(int subpassID, VkRenderPass renderPass)
        {
            instance = new BaseRenderer();
            instance->subpassID = subpassID;
            instance->renderPass = renderPass;
            instance->environment = RenderEnvironment::GetInstance();
            instance->createGlobalDescriptorSet();
            instance->createSkyBox();
            return instance;
        }

        static void Dispose()
        {
            vkDestroyPipeline(instance->environment->logicalDevice, instance->renderInfo->pipeline, nullptr);
            vkDestroyPipelineLayout(instance->environment->logicalDevice, instance->renderInfo->pipelineLayout, nullptr);
        }

        static void RegisterCamera(VkBuffer buffer)
        {
            DescriptorInfo &info = instance->globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = info.bufferSize;
            VkWriteDescriptorSet descriptorWrite = ConstructDescriptorSetWrite(instance->globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }

        void Render(VkCommandBuffer commandBuffer);

    private:
        RenderEnvironment *environment;
        RenderInfo *renderInfo;

        void createGlobalDescriptorSet();
        void createSkyBox();
        void createGraphicsPipeline();
    };
}

#endif