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

namespace vke_component
{
    vke_common::SceneResult<vke_common::TypeInfoDataPtr> ScriptStateData::PrepareForInstantiation() const
    {
        using namespace vke_common;
        auto *manager = ScriptManager::GetInstance();
        if (!manager) return std::unexpected("ScriptManager is not initialized");
        auto type = manager->FindTypeInfo(className);
        if (!type) return std::unexpected("missing script TypeInfo: " + className);
        auto encoded = type->EncodeBinaryFromJson(serializedData);
        if (!encoded)
            return std::unexpected(className + " at " + encoded.error().path + ": " + std::string(ToString(encoded.error().code)));
        if ((*encoded)->size() > INT32_MAX)
            return std::unexpected("script data exceeds interop size limit");
        return std::move(*encoded);
    }
}

namespace vke_common
{
    SceneResult<void> SceneData::loadComponent(const entt::entity entity,
                                               const nlohmann::json &component)
    {
        const std::string type = component["type"];
        auto load = [&]<typename T>() -> SceneResult<void>
        {
            if (auto valid = T::ValidateJSON(component); !valid) return valid;
            registry.emplace<T>(entity, component);
            return {};
        };
        if (type == "camera") return load.operator()<vke_component::CameraData>();
        if (type == "renderableObject") return load.operator()<vke_component::RenderableObjectData>();
        if (type == "uiText") return load.operator()<vke_component::UITextData>();
        if (type == "animator") return load.operator()<vke_component::SkeletonAnimatorData>();
        if (type == "rigidbody") return load.operator()<vke_component::RigidBodyData>();
        if (type == "sensor") return load.operator()<vke_component::SensorData>();
        if (type == "characterController") return load.operator()<vke_component::CharacterControllerData>();
        if (type == "audioSource") return load.operator()<vke_component::AudioSourceData>();
        if (type == "audioListener") return load.operator()<vke_component::AudioListenerData>();
        if (type == "directionalLight") return load.operator()<vke_component::DirectionalLightData>();
        if (type == "pointLight") return load.operator()<vke_component::PointLightData>();
        if (type == "spotLight") return load.operator()<vke_component::SpotLightData>();
        if (type == "script")
        {
            if (auto valid = vke_component::ScriptStateData::ValidateJSON(component); !valid) return valid;
            if (!registry.all_of<ScriptDataList>(entity)) registry.emplace<ScriptDataList>(entity);
            registry.get<ScriptDataList>(entity).emplace_back(component);
            return {};
        }
        return std::unexpected("unknown component type: " + type);
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
