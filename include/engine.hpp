#ifndef ENGINE_H
#define ENGINE_H

#include <render/render.hpp>
#include <physics/physics.hpp>
#include <scene.hpp>
#include <event.hpp>
#include <time.hpp>
#include <logger.hpp>

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

        static Engine *Init(GLFWwindow *window,
                            vke_render::RenderContext *ctx,
                            std::vector<vke_render::PassType> &passes,
                            std::vector<std::unique_ptr<vke_render::RenderPassBase>> &customPasses)
        {
            instance = new Engine();
            Logger::Init();
            EventSystem::Init();
            TimeManager::Init();
            vke_render::RenderEnvironment::Init(window);
            AssetManager::Init();
            vke_physics::PhysicsManager::Init();
            vke_render::DescriptorSetAllocator::Init();
            if (ctx == nullptr)
                ctx = &(vke_render::RenderEnvironment::GetInstance()->rootRenderContext);
            vke_render::Renderer::Init(ctx, passes, customPasses);
            SceneManager::Init();
            return instance;
        }

        static void WaitIdle()
        {
            vke_render::Renderer::WaitIdle();
        }

        static void Dispose()
        {
            SceneManager::Dispose();
            vke_render::Renderer::Dispose();
            vke_render::DescriptorSetAllocator::Dispose();
            vke_physics::PhysicsManager::Dispose();
            AssetManager::Dispose();
            vke_render::RenderEnvironment::Dispose();
            TimeManager::Dispose();
            EventSystem::Dispose();
            Logger::Dispose();
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