#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <render/environment.hpp>

namespace vke_render
{
    class BaseRenderer
    {
    private:
        static BaseRenderer *instance;
        BaseRenderer() = default;
        ~BaseRenderer() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (BaseRenderer::instance != nullptr)
                    delete BaseRenderer::instance;
            }
        };
        static Deletor deletor;

    public:
        static BaseRenderer *Init()
        {
            instance = new BaseRenderer();
            return instance;
        }

    private:
        RenderEnvironment *environment;
    };
}

#endif