#ifndef ENGINE_H
#define ENGINE_H

#include <render/render.hpp>
#include <render/opaque_render.hpp>
#include <render/resource.hpp>
#include <render/descriptor.hpp>

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

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (Engine::instance != nullptr)
                    delete Engine::instance;
            }
        };
        static Deletor deletor;

    public:
        static Engine *GetInstance()
        {
            if (instance == nullptr)
                instance = new Engine();
            return instance;
        }

        vke_render::RenderEnvironment *environment;
        vke_render::RenderResourceManager *renderRM;
        vke_render::DescriptorSetAllocator *allocator;
        vke_render::Renderer *renderer;

        static Engine *Init(int width, int height)
        {
            instance = new Engine();
            instance->environment = vke_render::RenderEnvironment::Init(width, height);
            instance->renderRM = vke_render::RenderResourceManager::Init();
            instance->allocator = vke_render::DescriptorSetAllocator::Init();
            instance->renderer = vke_render::Renderer::Init();
            return instance;
        }

        static void Dispose()
        {
            vke_render::Renderer::Dispose();
            instance->environment->Dispose();
        }

        void MainLoop();
    };
};

#endif