#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <ds/id_allocator.hpp>
#include <string>

namespace vke_common
{
    class GameObject
    {
    public:
        vke_ds::id32_t id;
        int layer;
        bool isStatic;
        std::string name;

        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject &) = delete;

        GameObject(vke_ds::id32_t id, std::string &name, int layer, bool isStatic)
            : id(id), layer(layer), isStatic(isStatic), name(name) {}

        ~GameObject() {}
    };
}

#endif
