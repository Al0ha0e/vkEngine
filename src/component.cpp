#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/skeleton_animator.hpp>
#include <component/rigidbody.hpp>
#include <component/sensor.hpp>
#include <component/character_controller.hpp>
#include <component/light.hpp>
#include <component/text.hpp>
#include <component/script.hpp>
#include <scene.hpp>

namespace vke_common
{
    void SceneData::loadComponent(const entt::entity entity,
                                  const nlohmann::json &component)
    {
        std::string type = component["type"];
        if (type == "camera")
        {
            registry.emplace<vke_component::CameraData>(entity, component);
        }
        else if (type == "renderableObject")
        {
            registry.emplace<vke_component::RenderableObjectData>(entity, component);
        }
        else if (type == "uiText")
        {
            registry.emplace<vke_component::UITextData>(entity, component);
        }
        else if (type == "animator")
        {
            registry.emplace<vke_component::SkeletonAnimatorData>(entity, component);
        }
        else if (type == "rigidbody")
        {
            registry.emplace<vke_component::RigidBodyData>(entity, component);
        }
        else if (type == "sensor")
        {
            registry.emplace<vke_component::SensorData>(entity, component);
        }
        else if (type == "characterController")
        {
            registry.emplace<vke_component::CharacterControllerData>(entity, component);
        }
        else if (type == "audioSource")
        {
            registry.emplace<vke_component::AudioSourceData>(entity, component);
        }
        else if (type == "audioListener")
        {
            registry.emplace<vke_component::AudioListenerData>(entity, component);
        }
        else if (type == "directionalLight")
        {
            registry.emplace<vke_component::DirectionalLightData>(entity, component);
        }
        else if (type == "pointLight")
        {
            registry.emplace<vke_component::PointLightData>(entity, component);
        }
        else if (type == "spotLight")
        {
            registry.emplace<vke_component::SpotLightData>(entity, component);
        }
        else if (type == "script")
        {
            if (!registry.all_of<ScriptDataList>(entity))
                registry.emplace<ScriptDataList>(entity);
            registry.get<ScriptDataList>(entity).emplace_back(component);
        }
    }

    void SceneData::componentToJSON(entt::entity entity, nlohmann::json &components) const
    {
        if (registry.all_of<vke_component::CameraData>(entity))
            components.push_back(registry.get<vke_component::CameraData>(entity).ToJSON());

        if (registry.all_of<vke_component::RenderableObjectData>(entity))
            components.push_back(registry.get<vke_component::RenderableObjectData>(entity).ToJSON());

        if (registry.all_of<vke_component::UITextData>(entity))
            components.push_back(registry.get<vke_component::UITextData>(entity).ToJSON());

        if (registry.all_of<vke_component::SkeletonAnimatorData>(entity))
            components.push_back(registry.get<vke_component::SkeletonAnimatorData>(entity).ToJSON());

        if (registry.all_of<vke_component::RigidBodyData>(entity))
            components.push_back(registry.get<vke_component::RigidBodyData>(entity).ToJSON());

        if (registry.all_of<vke_component::SensorData>(entity))
            components.push_back(registry.get<vke_component::SensorData>(entity).ToJSON());

        if (registry.all_of<vke_component::CharacterControllerData>(entity))
            components.push_back(registry.get<vke_component::CharacterControllerData>(entity).ToJSON());

        if (registry.all_of<vke_component::AudioSourceData>(entity))
            components.push_back(registry.get<vke_component::AudioSourceData>(entity).ToJSON());

        if (registry.all_of<vke_component::AudioListenerData>(entity))
            components.push_back(registry.get<vke_component::AudioListenerData>(entity).ToJSON());

        if (registry.all_of<vke_component::DirectionalLightData>(entity))
            components.push_back(registry.get<vke_component::DirectionalLightData>(entity).ToJSON());

        if (registry.all_of<vke_component::PointLightData>(entity))
            components.push_back(registry.get<vke_component::PointLightData>(entity).ToJSON());

        if (registry.all_of<vke_component::SpotLightData>(entity))
            components.push_back(registry.get<vke_component::SpotLightData>(entity).ToJSON());

        if (registry.all_of<ScriptDataList>(entity))
            for (const vke_component::ScriptStateData &script :
                 registry.get<ScriptDataList>(entity))
                components.push_back(script.ToJSON());
    }

    bool SceneManager::HasComponent(entt::entity entity, ComponentType componentType) const
    {
        if (!registry.valid(entity))
            return false;

        switch (componentType)
        {
        case ComponentType::Transform:
            return registry.all_of<vke_common::Transform>(entity);
        case ComponentType::Camera:
            return registry.all_of<vke_component::Camera>(entity);
        case ComponentType::RenderableObject:
            return registry.all_of<vke_component::RenderableObject>(entity);
        case ComponentType::SkeletonAnimator:
            return registry.all_of<vke_component::SkeletonAnimator>(entity);
        case ComponentType::RigidBody:
            return registry.all_of<vke_component::RigidBody>(entity);
        case ComponentType::Sensor:
            return registry.all_of<vke_component::Sensor>(entity);
        case ComponentType::CharacterController:
            return registry.all_of<vke_component::CharacterController>(entity);
        case ComponentType::DirectionalLight:
            return registry.all_of<vke_component::DirectionalLight>(entity);
        case ComponentType::PointLight:
            return registry.all_of<vke_component::PointLight>(entity);
        case ComponentType::SpotLight:
            return registry.all_of<vke_component::SpotLight>(entity);
        case ComponentType::Script:
            return csharpScriptStates.find(entity) != csharpScriptStates.end();
        case ComponentType::UIText:
            return registry.all_of<vke_component::UIText>(entity);
        case ComponentType::AudioSource:
            return registry.all_of<vke_component::AudioSource>(entity);
        case ComponentType::AudioListener:
            return registry.all_of<vke_component::AudioListener>(entity);
        default:
            return false;
        }
    }
}
