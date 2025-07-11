#ifndef OPAQUE_RENDER_H
#define OPAQUE_RENDER_H

#include <render/renderinfo.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class OpaqueRenderer : public RenderPassBase
    {
    public:
        OpaqueRenderer(RenderContext *ctx, VkDescriptorSet globalDescriptorSet)
            : RenderPassBase(OPAQUE_RENDERER, ctx, globalDescriptorSet) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
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

        uint64_t AddUnit(std::shared_ptr<Material> &material, std::shared_ptr<const Mesh> &mesh, void *pushConstants, uint32_t constantSize)
        {
            RegisterMaterial(material);
            return renderInfoMap[material.get()]->AddUnit(mesh, pushConstants, constantSize);
        }

        void RemoveUnit(Material *material, vke_ds::id64_t id)
        {
            renderInfoMap[material]->RemoveUnit(id);
            // TODO renderInfoMap[material].size() == 0
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        std::map<Material *, std::unique_ptr<RenderInfo>> renderInfoMap;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createGraphicsPipeline(RenderInfo &renderInfo);
    };
}

#endif