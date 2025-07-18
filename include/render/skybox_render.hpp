#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <render/environment.hpp>
#include <render/renderinfo.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/subpass.hpp>
#include <render/pipeline.hpp>

namespace vke_render
{
    class BaseRenderer : public RenderPassBase
    {
    public:
        BaseRenderer(RenderContext *ctx, VkDescriptorSet globalDescriptorSet)
            : RenderPassBase(SKYBOX_RENDERER, ctx, globalDescriptorSet) {}

        ~BaseRenderer() {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            createSkyBox();
            createGraphicsPipeline();
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        VkDescriptorSet skyBoxDescriptorSet;
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::unique_ptr<Mesh> skyboxMesh;
        std::unique_ptr<Material> skyboxMaterial;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createSkyBox();
        void createGraphicsPipeline();
    };
}

#endif