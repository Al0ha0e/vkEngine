#ifndef RENDERER_H
#define RENDERER_H

#include <common.hpp>
#include <render/environment.hpp>
#include <vector>
#include <map>

namespace vke_render
{
    class Renderer
    {
    private:
        static Renderer *instance;
        Renderer() = default;
        ~Renderer() {}

        class Deletor
        {
        public:
            ~Deletor()
            {
                if (Renderer::instance != nullptr)
                    delete Renderer::instance;
            }
        };
        static Deletor deletor;

    public:
        static Renderer *Init()
        {
            instance = new Renderer();
            return instance;
        }
    };
}

#endif