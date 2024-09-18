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

        OpaqueRenderer(RenderContext *ctx) : globalDescriptorSetInfo(nullptr, 0, 0, 0), SubpassBase(OPAQUE_RENDERER, ctx) {}

        void Init(int subpassID, VkRenderPass renderPass) override
        {
            SubpassBase::Init(subpassID, renderPass);
            environment = RenderEnvironment::GetInstance();
            createGlobalDescriptorSet();
        }

        ~OpaqueRenderer() {}

        void RegisterCamera(VkBuffer buffer) override
        {
            DescriptorInfo &info = globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            InitDescriptorBufferInfo(bufferInfo, buffer, 0, info.bufferSize);
            VkWriteDescriptorSet descriptorWrite = ConstructDescriptorSetWrite(globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }

        void RegisterMaterial(std::shared_ptr<Material> &material)
        {
            // uint32_t id = instance->materialIDAllocator.Alloc();
            auto &rMap = renderInfoMap;
            Material *matp = material.get();
            if (rMap.find(matp) != rMap.end())
                return;

            RenderInfo *info = new RenderInfo(material);
            createGraphicsPipeline(*info);
            rMap[matp] = std::move(std::unique_ptr<RenderInfo>(info));
            // return id;
        }

        uint64_t AddUnit(std::shared_ptr<Material> &material, std::shared_ptr<const Mesh> &mesh, std::vector<std::unique_ptr<vke_render::HostCoherentBuffer>> &buffers)
        {
            RegisterMaterial(material);
            return renderInfoMap[material.get()]->AddUnit(mesh, buffers);
        }

        void Render(VkCommandBuffer commandBuffer) override;

    private:
        RenderEnvironment *environment;
        std::map<Material *, std::unique_ptr<RenderInfo>> renderInfoMap;

        void createGlobalDescriptorSet();
        void createGraphicsPipeline(RenderInfo &renderInfo);
    };
}

#endif