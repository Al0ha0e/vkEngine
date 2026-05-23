#ifndef SKYBOX_RENDER_H
#define SKYBOX_RENDER_H

#include <render/skybox.hpp>
#include <render/renderinfo.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class SkyboxRenderer : public RenderPassBase
    {
    public:
        SkyboxRenderer(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, SkyboxManager *skyboxManager)
            : RenderPassBase(SKYBOX_RENDERER, ctx, globalDescriptorSets), skyboxManager(skyboxManager) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
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

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void initResources();
        void createDescriptorSet();
        void createGraphicsPipeline();
    };
}

#endif