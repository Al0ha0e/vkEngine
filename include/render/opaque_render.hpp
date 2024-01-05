#ifndef OPAQUE_RENDER_H
#define OPAQUE_RENDER_H

#include <render/renderinfo.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class OpaqueRenderer : public SubpassBase
    {
    public:
        std::vector<DescriptorInfo> globalDescriptorInfos;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;

        OpaqueRenderer() {}

        void Init(int subpassID, VkRenderPass renderPass) override
        {
            SubpassBase::Init(subpassID, renderPass);
            environment = RenderEnvironment::GetInstance();
            createGlobalDescriptorSet();
        }

        void Dispose() override
        {
            for (auto &renderInfo : renderInfoMap)
            {
                vkDestroyPipeline(environment->logicalDevice, renderInfo.second->pipeline, nullptr);
                vkDestroyPipelineLayout(environment->logicalDevice, renderInfo.second->pipelineLayout, nullptr);
            }
        }

        void RegisterCamera(VkBuffer buffer) override
        {
            DescriptorInfo &info = globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            InitDescriptorBufferInfo(bufferInfo, buffer, 0, info.bufferSize);
            VkWriteDescriptorSet descriptorWrite = ConstructDescriptorSetWrite(globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }

        void RegisterMaterial(Material *material)
        {
            // uint32_t id = instance->materialIDAllocator.Alloc();
            auto &rMap = renderInfoMap;
            if (rMap.find(material) != rMap.end())
                return;

            RenderInfo *info = new RenderInfo(material);
            createGraphicsPipeline(*info);
            rMap[material] = info;
            // return id;
        }

        uint64_t AddUnit(Material *material, Mesh *mesh, std::vector<HostCoherentBuffer> &buffers)
        {
            RegisterMaterial(material);
            return renderInfoMap[material]->AddUnit(mesh, buffers);
        }

        void Render(VkCommandBuffer commandBuffer) override;

    private:
        RenderEnvironment *environment;
        std::map<Material *, RenderInfo *> renderInfoMap;

        void createGlobalDescriptorSet();
        void createGraphicsPipeline(RenderInfo &renderInfo);
    };
}

#endif