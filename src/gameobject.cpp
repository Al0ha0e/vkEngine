#include <gameobject.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>

namespace vke_common
{
    std::unique_ptr<Component> GameObject::loadComponent(nlohmann::json &json)
    {
        std::string type = json["type"];
        if (type == "camera")
            return std::make_unique<vke_component::Camera>(this, json);
        if (type == "renderableObject")
            return std::make_unique<vke_component::RenderableObject>(this, json);
        return nullptr;
    }
}