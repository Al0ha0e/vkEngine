#ifndef SKYBOX_RENDER_H
#define SKYBOX_RENDER_H

#include <render/skybox.hpp>
#include <render/renderinfo.hpp>
#include <render/subpass.hpp>
#include <render/hdr_color.hpp>

namespace vke_render
{
    class SkyboxRenderer : public RenderPassBase
    {
    public:
        SkyboxRenderer(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, SkyboxManager *skyboxManager, HDRColorManager *hdrColorManager)
            : RenderPassBase(SKYBOX_RENDERER, ctx, globalDescriptorSets), skyboxManager(skyboxManager), hdrColorManager(hdrColorManager) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  ResourceNodeIDMap &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            initResources();
            createDescriptorSet();
            createGraphicsPipeline();
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        VkDescriptorSet skyBoxDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::unique_ptr<Mesh> skyboxMesh;
        std::shared_ptr<Material> skyboxMaterial;
        SkyboxManager *skyboxManager;
        HDRColorManager *hdrColorManager;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 ResourceNodeIDMap &currentResourceNodeID);
        void initResources();
        void createDescriptorSet();
        void createGraphicsPipeline();
    };
}

#endif
