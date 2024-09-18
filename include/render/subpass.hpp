#ifndef SUBPASS_H
#define SUBPASS_H

#include <render/environment.hpp>

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

    class SubpassBase
    {
    public:
        PassType type;
        int subpassID;
        RenderContext *context;
        VkRenderPass renderPass;

        SubpassBase(PassType type, RenderContext *ctx) : type(type), context(ctx) {}

        virtual ~SubpassBase() {}

        virtual void RegisterCamera(VkBuffer buffer) = 0;
        virtual void Render(VkCommandBuffer commandBuffer) = 0;

        virtual void Init(int subpassID, VkRenderPass renderPass)
        {
            this->subpassID = subpassID;
            this->renderPass = renderPass;
        }
    };
}

#endif