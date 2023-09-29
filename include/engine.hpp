#ifndef ENGINE_H
#define ENGINE_H

#include <render/render.hpp>

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
        vke_render::OpaqueRenderer *opaq_renderer;

        void Init(int width, int height)
        {
            environment = vke_render::RenderEnvironment::GetInstance();
            environment->Init(width, height);
            opaq_renderer = vke_render::OpaqueRenderer::GetInstance();
            opaq_renderer->Init();
        }

        void Dispose()
        {
            opaq_renderer->Dispose();
            environment->Dispose();
        }

        void MainLoop();
    };
};

#endif