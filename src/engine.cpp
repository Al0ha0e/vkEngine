#include <game_config.hpp>
#include <engine.hpp>

namespace vke_common
{
    GameConfig *GameConfig::instance = nullptr;
    Engine *Engine::instance = nullptr;

    bool Engine::Update()
    {
        const EngineState state = vke_common::EngineStateManager::GetState();
        if (state == EngineState::Terminated)
            return false;

        vke_common::TimeManager::Update();

        if (state == EngineState::Paused)
        {
            vke_render::Renderer::GetInstance()->Update();
            vke_common::InputManager::EndFrame();
            return true;
        }

        vke_common::ScriptManager::GetInstance()->Update();
        vke_physics::PhysicsManager::Update();
        vke_render::Renderer::GetInstance()->Update();
        vke_common::InputManager::EndFrame();
        return true;
    }

    void Engine::MainLoop()
    {
        while (!glfwWindowShouldClose(vke_render::RenderEnvironment::GetInstance()->window))
        {
            glfwPollEvents();
            Update();
        }
        vkDeviceWaitIdle(vke_render::globalLogicalDevice);
    }
}
