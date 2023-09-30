#include <engine.hpp>

namespace vke_common
{
    Engine *Engine::instance = nullptr;

    void Engine::MainLoop()
    {
        while (!glfwWindowShouldClose(environment->window))
        {
            glfwPollEvents();
            opaqRenderer->Update();
        }
        vkDeviceWaitIdle(environment->logicalDevice);
    }
}