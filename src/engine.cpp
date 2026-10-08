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
        {
            Shutdown();
            return false;
        }

        vke_common::TimeManager::Update();
        vke_common::SceneManager::GetInstance()->ProcessDestroyRequests();

        if (state == EngineState::Paused)
        {
            vke_render::Renderer::GetInstance()->Update();
            vke_common::InputManager::EndFrame();
            return true;
        }

        UpdateSimulation(vke_common::TimeManager::GetDeltaTime(), fixedUpdateAccumulator);
        vke_render::Renderer::GetInstance()->Update();
        vke_common::InputManager::EndFrame();
        return true;
    }

    void Engine::UpdateSimulation(float deltaTime, float &fixedUpdateAccumulator)
    {
        vke_common::SceneManager::GetInstance()->Update(deltaTime);
        fixedUpdateAccumulator += deltaTime;
        const float fixedStepTime = vke_physics::PhysicsManager::GetConfig().stepTime;
        while (fixedUpdateAccumulator >= fixedStepTime)
        {
            FixedUpdate(fixedStepTime);
            fixedUpdateAccumulator -= fixedStepTime;
        }
        vke_common::SceneManager::GetInstance()->LateUpdate(deltaTime);
        if (vke_common::SceneManager::IsRunning())
            vke_audio::AudioManager::Update(deltaTime);
    }

    void Engine::FixedUpdate(float deltaTime)
    {
        vke_common::SceneManager::GetInstance()->FixedUpdate(deltaTime);
        if (vke_common::SceneManager::IsRunning())
            vke_physics::PhysicsManager::FixedUpdate(deltaTime);
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
