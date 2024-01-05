#ifndef SUBPASS_H
#define SUBPASS_H

#include <render/renderinfo.hpp>

namespace vke_render
{
    class SubpassBase
    {
    public:
        int subpassID;
        VkRenderPass renderPass;

        SubpassBase() {}

        virtual void Dispose() = 0;
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