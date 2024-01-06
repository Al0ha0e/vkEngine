#include <engine.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vke_common
{
    Engine *Engine::instance = nullptr;

    void Engine::Update()
    {
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