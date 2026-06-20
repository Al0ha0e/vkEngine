#ifndef SHADOW_PASS_H
#define SHADOW_PASS_H

#include <render/renderinfo.hpp>
#include <render/subpass.hpp>
#include <render/shadow_manager.hpp>
#include <asset.hpp>

namespace vke_render
{
    class ShadowPass : public RenderPassBase
    {
    public:
        ShadowPass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, ShadowManager *shadowManager)
            : RenderPassBase(SHADOW_PASS, ctx, globalDescriptorSets), shadowManager(shadowManager),
              shadowTaskNodeID(0), unitAllocator(1) {}

        ~ShadowPass() {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  ResourceNodeIDMap &currentResourceNodeID) override;

        vke_ds::id64_t AddUnit(RenderUnit *unit, bool isSkin = false);
        void RemoveUnit(vke_ds::id64_t id);

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override;

    private:
        ShadowManager *shadowManager;
        std::shared_ptr<Material> shadowMaterial;
        std::shared_ptr<Material> shadowSkinMaterial;
        std::map<Material *, std::unique_ptr<RenderInfo>> renderInfoMap;
        std::map<vke_ds::id64_t, Material *> unitMaterialMap;
        vke_ds::id32_t shadowTaskNodeID;
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> unitAllocator;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 ResourceNodeIDMap &currentResourceNodeID);
        void createGraphicsPipeline(RenderInfo &renderInfo, bool isSkin);
        void registerMaterial(std::shared_ptr<Material> &material, bool isSkin);
    };
}

#endif
