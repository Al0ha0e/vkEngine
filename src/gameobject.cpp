#include <gameobject.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/rigidbody.hpp>

namespace vke_common
{
    std::unique_ptr<Component> GameObject::loadComponent(const nlohmann::json &json)
    {
        std::string type = json["type"];
        if (type == "camera")
            return std::make_unique<vke_component::Camera>(this, json);
        if (type == "renderableObject")
            return std::make_unique<vke_component::RenderableObject>(this, json);
        if (type == "rigidbody")
            return std::make_unique<vke_physics::RigidBody>(this, json);
        return nullptr;
    }
}