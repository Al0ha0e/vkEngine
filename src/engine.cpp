#include <engine.hpp>

namespace vke_common
{
    Engine *Engine::instance = nullptr;

    void Engine::MainLoop()
    {
        while (!glfwWindowShouldClose(environment->window))
        {
            glfwPollEvents();
            opaq_renderer->Update();
        }
        vkDeviceWaitIdle(environment->logicalDevice);
    }
}