#ifndef ENGINE_H
#define ENGINE_H

#include <render/render.hpp>
#include <render/opaque_render.hpp>
#include <resource.hpp>
#include <render/descriptor.hpp>
#include <scene.hpp>
#include <event.hpp>

namespace vke_common
{
    class Engine
    {
    private:
        static Engine *instance;
        Engine() {}
        ~Engine() {}
        Engine(const Engine &);
        Engine &operator=(const Engine);

    public:
        static Engine *GetInstance()
        {
            if (instance == nullptr)
                throw std::runtime_error("Engine not initialized!");
            return instance;
        }

        vke_render::RenderEnvironment *environment;
        vke_common::ResourceManager *resourceManager;
        vke_render::DescriptorSetAllocator *allocator;
        vke_render::Renderer *renderer;

        static Engine *Init(vke_render::RenderContext *ctx,
                            std::vector<vke_render::PassType> &passes,
                            std::vector<std::unique_ptr<vke_render::SubpassBase>> &customPasses,
                            std::vector<vke_render::RenderPassInfo> &customPassInfo)
        {
            vke_render::RenderEnvironment *environment = vke_render::RenderEnvironment::GetInstance();
            instance = new Engine();
            instance->environment = environment;
            instance->resourceManager = vke_common::ResourceManager::Init();
            instance->allocator = vke_render::DescriptorSetAllocator::Init();
            instance->renderer = vke_render::Renderer::Init(ctx, passes, customPasses, customPassInfo);
            SceneManager::Init();
            return instance;
        }

        static void Dispose()
        {
            SceneManager::Dispose();
            vke_render::Renderer::Dispose();
            vke_render::DescriptorSetAllocator::Dispose();
            vke_common::ResourceManager::Dispose();
            instance->environment->Dispose();
            EventSystem::Dispose();
            delete instance;
        }

        static void OnWindowResize(GLFWwindow *window, int width, int height)
        {
            EventSystem::DispatchEvent(EVENT_WINDOW_RESIZE, nullptr);
        }

        void Update();

        void MainLoop();
    };
};

#endif