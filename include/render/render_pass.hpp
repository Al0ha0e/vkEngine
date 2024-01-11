#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <render/environment.hpp>

namespace vke_render
{

    struct RenderPassInfo
    {
        VkPipelineStageFlags stageMask;
        VkAccessFlags accessMask;

        RenderPassInfo() {}
        RenderPassInfo(VkPipelineStageFlags stageMask, VkAccessFlags accessMask)
            : stageMask(stageMask), accessMask(accessMask) {}
    };

    class RenderPasses
    {
    private:
        static RenderPasses *instance;
        int passcnt;
        RenderPasses() = default;
        ~RenderPasses() {}

    public:
        VkRenderPass renderPass;

        static RenderPasses *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("RenderPasses not initialized!");
            return instance;
        }

        static RenderPasses *Init(std::vector<RenderPassInfo> &passInfo)
        {
            instance = new RenderPasses();
            instance->environment = RenderEnvironment::GetInstance();
            instance->passcnt = passInfo.size();
            instance->createRenderPass(passInfo);
            return instance;
        }

        static void Dispose()
        {
            vkDestroyRenderPass(instance->environment->logicalDevice, instance->renderPass, nullptr);
        }

        // void Update();

    private:
        RenderEnvironment *environment;
        void createRenderPass(std::vector<RenderPassInfo> &passInfo);
    };
}

#endif