#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>

namespace vke_render
{
    class Renderer
    {
    };

    class OpaqueRenderer
    {
    private:
        static OpaqueRenderer *instance;
        OpaqueRenderer() {}
        ~OpaqueRenderer() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (OpaqueRenderer::instance != nullptr)
                    delete OpaqueRenderer::instance;
            }
        };
        static Deletor deletor;

    public:
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        uint32_t currentFrame;

        static OpaqueRenderer *GetInstance()
        {
            if (instance == nullptr)
                instance = new OpaqueRenderer();
            return instance;
        }

        void Init()
        {
            currentFrame = 0;
            environment = RenderEnvironment::GetInstance();
            createRenderPass();
            createGraphicsPipeline();
            createFramebuffers();
        }

        void Dispose()
        {
            for (auto framebuffer : swapChainFramebuffers)
            {
                vkDestroyFramebuffer(environment->logicalDevice, framebuffer, nullptr);
            }
            vkDestroyPipeline(environment->logicalDevice, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(environment->logicalDevice, pipelineLayout, nullptr);
            vkDestroyRenderPass(environment->logicalDevice, renderPass, nullptr);
        }

        void Update();

    private:
        RenderEnvironment *environment;

        void createRenderPass();
        VkShaderModule createShaderModule(const std::vector<char> &code);
        void createGraphicsPipeline();
        void createFramebuffers();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void drawFrame();
    };
}

#endif