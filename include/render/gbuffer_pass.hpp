#ifndef GBUFFER_PASS_H
#define GUBFFER_PASS_H

#include <render/gbuffer.hpp>
#include <render/renderinfo.hpp>
#include <render/subpass.hpp>

namespace vke_render
{

    class GBufferPass : public RenderPassBase
    {
    public:
        GBufferPass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets)
            : RenderPassBase(GBUFFER_PASS, ctx, globalDescriptorSets) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            gbuffer = GBuffer::Init(context->width, context->height);
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
        }

        ~GBufferPass()
        {
            GBuffer::Dispose();
        }

        void RegisterMaterial(std::shared_ptr<Material> &material, bool isSkin = false)
        {
            auto &rMap = renderInfoMap;
            Material *matp = material.get();
            if (rMap.find(matp) != rMap.end())
                return;

            RenderInfo *info = new RenderInfo(material);
            createGraphicsPipeline(*info, isSkin);
            rMap[matp] = std::move(std::unique_ptr<RenderInfo>(info));
            // return id;
        }

        vke_ds::id64_t AddUnit(std::shared_ptr<Material> &material, RenderUnit *unit, bool isSkin = false)
        {
            RegisterMaterial(material, isSkin);
            return renderInfoMap[material.get()]->AddUnit(unit);
        }

        void RemoveUnit(Material *material, vke_ds::id64_t id)
        {
            renderInfoMap[material]->RemoveUnit(id);
            // TODO renderInfoMap[material].size() == 0
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        std::map<Material *, std::unique_ptr<RenderInfo>> renderInfoMap;
        GBuffer *gbuffer;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createGraphicsPipeline(RenderInfo &renderInfo, bool isSkin);
    };
}

#endif