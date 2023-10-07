#include <engine.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vke_common
{
    Engine *Engine::instance = nullptr;

    void Engine::MainLoop()
    {
        while (!glfwWindowShouldClose(environment->window))
        {
            glfwPollEvents();
            renderer->Update();
        }
        vkDeviceWaitIdle(environment->logicalDevice);
    }
}