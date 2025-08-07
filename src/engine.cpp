#include <engine.hpp>

namespace vke_common
{
    Engine *Engine::instance = nullptr;

    void Engine::Update()
    {
        vke_physics::PhysicsManager::Update();
        renderer->Update();
    }

    void Engine::MainLoop()
    {
        while (!glfwWindowShouldClose(environment->window))
        {
            glfwPollEvents();
            Update();
        }
        vkDeviceWaitIdle(environment->logicalDevice);
    }
}