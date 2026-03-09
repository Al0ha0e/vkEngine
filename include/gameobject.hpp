#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <nlohmann/json.hpp>
#include <ds/id_allocator.hpp>

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

        GameObject(const nlohmann::json &json)
            : id(json["id"]), layer(json["layer"]), isStatic(json["static"]), name(json["name"]) {}

        ~GameObject() {}

        nlohmann::json ToJSON()
        {
            nlohmann::json ret;
            ret["id"] = id;
            ret["static"] = isStatic;
            ret["name"] = name;
            ret["layer"] = layer;
            return ret;
        }
    };
}

#endif