#ifndef ATMOSPHERE_PASS_H
#define ATMOSPHERE_PASS_H

#include <asset.hpp>
#include <render/gbuffer.hpp>
#include <render/hdr_color.hpp>
#include <render/pipeline.hpp>
#include <render/skybox.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class AtmospherePass : public RenderPassBase
    {
    public:
        AtmospherePass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets,
                       SkyboxManager *skyboxManager, HDRColorManager *hdrColorManager);
        ~AtmospherePass() override = default;

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  ResourceNodeIDMap &currentResourceNodeID) override;
        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer,
                    uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override;

    private:
        SkyboxManager *skyboxManager;
        HDRColorManager *hdrColorManager;
        GBuffer *gbuffer;
        std::shared_ptr<ShaderModuleSet> shader;
        std::unique_ptr<GraphicsPipeline> pipeline;
        VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
        vke_ds::id32_t taskNodeID;
        uint32_t inputHDRColorImageIndex;
        uint32_t outputHDRColorImageIndex;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 ResourceNodeIDMap &currentResourceNodeID);
        void createDescriptorSet();
        void createGraphicsPipeline();
        void updateDescriptorSet(uint32_t currentFrame);
        void onTransientResourcesReady(uint32_t currentFrame);
    };
}

#endif
