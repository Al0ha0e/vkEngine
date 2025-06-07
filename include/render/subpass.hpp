#ifndef SUBPASS_H
#define SUBPASS_H

#include <render/environment.hpp>
#include <render/frame_graph.hpp>

namespace vke_render
{

    enum PassType
    {
        CUSTOM_RENDERER,
        BASE_RENDERER,
        OPAQUE_RENDERER
    };

    struct RenderPassInfo
    {
        VkPipelineStageFlags stageMask;
        VkAccessFlags accessMask;

        RenderPassInfo() {}
        RenderPassInfo(VkPipelineStageFlags stageMask, VkAccessFlags accessMask)
            : stageMask(stageMask), accessMask(accessMask) {}
    };

    class RenderPassBase
    {
    public:
        PassType type;
        int subpassID;
        RenderContext *context;
        VkBuffer camInfoBuffer;

        RenderPassBase(PassType type, RenderContext *ctx, VkBuffer camBuffer)
            : type(type), context(ctx), camInfoBuffer(camBuffer) {}

        virtual ~RenderPassBase() {}

        // virtual void RegisterCamera(VkBuffer buffer) = 0;
        virtual void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) = 0;

        virtual void Init(int subpassID)
        {
            this->subpassID = subpassID;
        }
    };
}

#endif