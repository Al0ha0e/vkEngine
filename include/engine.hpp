#ifndef ENGINE_H
#define ENGINE_H

#include <render/render.hpp>
#include <render/opaque_render.hpp>
#include <render/resource.hpp>
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
        vke_render::RenderResourceManager *renderRM;
        vke_render::DescriptorSetAllocator *allocator;
        vke_render::Renderer *renderer;

        static Engine *Init(
            int width,
            int height,
            std::vector<vke_render::PassType> &passes,
            std::vector<std::unique_ptr<vke_render::SubpassBase>> &customPasses,
            std::vector<vke_render::RenderPassInfo> &customPassInfo)
        {
            instance = new Engine();
            EventSystem::Init();
            instance->environment = vke_render::RenderEnvironment::Init(width, height);
            instance->renderRM = vke_render::RenderResourceManager::Init();
            instance->allocator = vke_render::DescriptorSetAllocator::Init();
            instance->renderer = vke_render::Renderer::Init(passes, customPasses, customPassInfo);
            SceneManager::Init();
            return instance;
        }

        static void Dispose()
        {
            SceneManager::Dispose();
            vke_render::Renderer::Dispose();
            vke_render::DescriptorSetAllocator::Dispose();
            vke_render::RenderResourceManager::Dispose();
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