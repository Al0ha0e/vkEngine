#ifndef TONE_MAPPING_H
#define TONE_MAPPING_H

#include <render/pipeline.hpp>
#include <render/subpass.hpp>
#include <render/hdr_color.hpp>
#include <asset.hpp>

namespace vke_render
{
    class ToneMappingPass : public RenderPassBase
    {
    public:
        ToneMappingPass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, HDRColorManager *hdrColorManager)
            : RenderPassBase(TONE_MAPPING_PASS, ctx, globalDescriptorSets), hdrColorManager(hdrColorManager) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  CurrentResourceNodeIDMaps &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            toneMappingShader = vke_common::AssetManager::LoadVertFragShaderUnique(vke_common::BUILTIN_VFSHADER_TONE_MAPPING_ID);
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            allocateDescriptorSet();
            createGraphicsPipeline();
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override {}

    private:
        struct ToneMappingConstants
        {
            float exposure;
            uint32_t toneMappingMode;
        };

        VkDescriptorSet toneMappingDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::shared_ptr<ShaderModuleSet> toneMappingShader;
        HDRColorManager *hdrColorManager;
        vke_ds::id32_t toneMappingTaskNodeID;
        ToneMappingConstants constants{1.0f, 0};

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 CurrentResourceNodeIDMaps &currentResourceNodeID);
        void allocateDescriptorSet();
        void createGraphicsPipeline();
        void onTransientResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame);
    };
}

#endif
