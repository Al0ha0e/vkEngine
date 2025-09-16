#include <engine.hpp>

namespace vke_common
{
    Engine *Engine::instance = nullptr;

    void Engine::Update()
    {
        vke_common::TimeManager::Update();
        vke_physics::PhysicsManager::Update();
        vke_render::Renderer::GetInstance()->Update();
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