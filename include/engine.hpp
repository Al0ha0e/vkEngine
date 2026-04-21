#ifndef ENGINE_H
#define ENGINE_H

#include <game_config.hpp>
#include <render/render.hpp>
#include <physics/physics.hpp>
#include <scene.hpp>
#include <event.hpp>
#include <input.hpp>
#include <engine_state.hpp>
#include <time.hpp>
#include <script.hpp>

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
            VKE_FATAL_IF(instance == nullptr, "Engine not initialized!")
            return instance;
        }

        static Engine *Init(GLFWwindow *window,
                            vke_render::RenderContext *ctx,
                            std::vector<vke_render::PassType> &passes,
                            std::vector<std::unique_ptr<vke_render::RenderPassBase>> &customPasses)
        {
            instance = new Engine();
            EventSystem::Init();
            TimeManager::Init();
            InputManager::Init(window);
            EngineStateManager::Init();
            vke_render::RenderEnvironment::Init(window);
            AssetManager::Init();
            vke_physics::PhysicsManager::Init();
            vke_render::DescriptorSetAllocator::Init();
            if (ctx == nullptr)
                ctx = &(vke_render::RenderEnvironment::GetInstance()->rootRenderContext);
            vke_render::Renderer::Init(ctx, passes, customPasses);
            ScriptManager::Init();
            SceneManager::Init();
            return instance;
        }

        static void Shutdown()
        {
            vke_render::Renderer::Shutdown();
        }

        static void WaitIdle()
        {
            vke_render::Renderer::WaitIdle();
        }

        static void Dispose()
        {
            SceneManager::Dispose();
            ScriptManager::Dispose();
            vke_render::Renderer::Dispose();
            vke_render::DescriptorSetAllocator::Dispose();
            vke_physics::PhysicsManager::Dispose();
            AssetManager::Dispose();
            vke_render::RenderEnvironment::Dispose();
            EngineStateManager::Dispose();
            InputManager::Dispose();
            TimeManager::Dispose();
            EventSystem::Dispose();
            delete instance;
        }

        static void OnWindowResize(GLFWwindow *window, int width, int height)
        {
            EventSystem::DispatchEvent(EVENT_WINDOW_RESIZE, nullptr);
        }

        bool Update();

        void MainLoop();
    };
};

#endif