#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>

namespace vke_common
{
    class GameObject
    {
    public:
        bool isStatic;
        std::string name;

        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject &) = delete;

        GameObject(GameObject &&) noexcept = default;
        GameObject &operator=(GameObject &&) noexcept = default;

        GameObject(std::string &name, bool isStatic)
            : isStatic(isStatic), name(name) {}

        ~GameObject() {}
    };
}

#endif
