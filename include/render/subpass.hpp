#ifndef SUBPASS_H
#define SUBPASS_H

#include <render/environment.hpp>
#include <render/frame_graph.hpp>

namespace vke_render
{

    enum PassType
    {
        CUSTOM_RENDERER,
        SKYBOX_RENDERER,
        OPAQUE_RENDERER
    };

    class RenderPassBase
    {
    public:
        PassType type;
        int subpassID;
        RenderContext *context;
        VkDescriptorSet globalDescriptorSet;

        RenderPassBase(PassType type, RenderContext *ctx, VkDescriptorSet globalDescriptorSet)
            : type(type), context(ctx), globalDescriptorSet(globalDescriptorSet) {}

        virtual ~RenderPassBase() {}

        // virtual void RegisterCamera(VkBuffer buffer) = 0;
        virtual void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) = 0;

        virtual void Init(int subpassID,
                          FrameGraph &frameGraph,
                          std::map<std::string, vke_ds::id32_t> &blackboard,
                          std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
        {
            this->subpassID = subpassID;
            environment = RenderEnvironment::GetInstance();
        }

    private:
        RenderEnvironment *environment;
    };
}

#endif