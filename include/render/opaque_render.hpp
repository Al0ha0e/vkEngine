#ifndef OPAQUE_RENDER_H
#define OPAQUE_RENDER_H

#include <render/renderinfo.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class OpaqueRenderer : public RenderPassBase
    {
    public:
        std::vector<DescriptorInfo> globalDescriptorInfos;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;

        OpaqueRenderer(RenderContext *ctx, VkBuffer camBuffer)
            : globalDescriptorSetInfo(nullptr, 0, 0, 0, 0), RenderPassBase(OPAQUE_RENDERER, ctx, camBuffer) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            environment = RenderEnvironment::GetInstance();
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            createGlobalDescriptorSet();
            registerCamera();
        }

        ~OpaqueRenderer() {}

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

        void RemoveUnit(Material *material, vke_ds::id64_t id)
        {
            renderInfoMap[material]->RemoveUnit(id);
            // TODO renderInfoMap[material].size() == 0
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        RenderEnvironment *environment;
        std::map<Material *, std::unique_ptr<RenderInfo>> renderInfoMap;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createGlobalDescriptorSet();
        void createGraphicsPipeline(RenderInfo &renderInfo);

        void registerCamera()
        {
            DescriptorInfo &info = globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            InitDescriptorBufferInfo(bufferInfo, camInfoBuffer, 0, info.bufferSize);
            VkWriteDescriptorSet descriptorWrite{};
            ConstructDescriptorSetWrite(descriptorWrite, globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }
    };
}

#endif