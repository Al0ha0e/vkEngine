#ifndef LIGHTING_PASS_H
#define LIGHTING_PASS_H

#include <render/gbuffer.hpp>
#include <render/renderinfo.hpp>
#include <render/subpass.hpp>
#include <asset.hpp>

namespace vke_render
{
    class DeferredLightingPass : public RenderPassBase
    {
    public:
        DeferredLightingPass(RenderContext *ctx, VkDescriptorSet globalDescriptorSet)
            : RenderPassBase(DEFERRED_LIGHTING_PASS, ctx, globalDescriptorSet) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            gbuffer = GBuffer::GetInstance();
            lightingShader = vke_common::AssetManager::LoadVertFragShaderUnique(vke_common::BUILTIN_VFSHADER_DEFERRED_LIGHTING_ID);
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            createDescriptorSet();
            createGraphicsPipeline();
        }

        ~DeferredLightingPass() {}

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        GBuffer *gbuffer;
        VkDescriptorSet lightingDescriptorSet;
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::shared_ptr<ShaderModuleSet> lightingShader;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createDescriptorSet();
        void createGraphicsPipeline();
    };
};

#endif