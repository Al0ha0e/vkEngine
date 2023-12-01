#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>
#include <render/base_render.hpp>
#include <render/opaque_render.hpp>
#include <render/render_pass.hpp>
#include <vector>
#include <map>

namespace vke_render
{
    class Renderer
    {
    private:
        static Renderer *instance;
        Renderer() = default;
        ~Renderer() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (Renderer::instance != nullptr)
                    delete Renderer::instance;
            }
        };
        static Deletor deletor;

    public:
        RenderPasses *renderPass;
        uint32_t currentFrame;
        BaseRenderer *baseRenderer;
        OpaqueRenderer *opaqueRenderer;

        static Renderer *GetInstance()
        {
            if (instance == nullptr)
                instance = new Renderer();
            return instance;
        }

        static Renderer *Init()
        {
            instance = new Renderer();
            instance->currentFrame = 0;
            instance->environment = RenderEnvironment::GetInstance();
            instance->initRenderPass();
            instance->createFramebuffers();
            instance->baseRenderer = BaseRenderer::Init(
                0,
                instance->renderPass->renderPass);
            instance->opaqueRenderer = OpaqueRenderer::Init(
                1,
                instance->renderPass->renderPass);

            return instance;
        }

        static void Dispose()
        {
            for (auto framebuffer : instance->frameBuffers)
            {
                vkDestroyFramebuffer(instance->environment->logicalDevice, framebuffer, nullptr);
            }
        }

        static void RegisterCamera(VkBuffer buffer)
        {
            instance->baseRenderer->RegisterCamera(buffer);
            instance->opaqueRenderer->RegisterCamera(buffer);
        }

        void Update();

    private:
        RenderEnvironment *environment;
        std::vector<VkFramebuffer> frameBuffers;

        void initRenderPass();
        void createFramebuffers();
        void render();
    };
}

#endif