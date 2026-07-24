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
    static inline glm::vec3 TransformForward(const vke_common::Transform &transform)
    {
        return transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void Scene::unloadEntityFromEngine(entt::entity entity)
    {
        if (!loadedToEngine)
            return;

        if (registry.all_of<vke_component::Camera>(entity))
            registry.get<vke_component::Camera>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::RenderableObject>(entity))
            registry.get<vke_component::RenderableObject>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::UIText>(entity))
            registry.get<vke_component::UIText>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::SkeletonAnimator>(entity))
            registry.get<vke_component::SkeletonAnimator>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::CharacterController>(entity))
            registry.get<vke_component::CharacterController>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::Sensor>(entity))
            registry.get<vke_component::Sensor>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::RigidBody>(entity))
            registry.get<vke_component::RigidBody>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::AudioSource>(entity))
            registry.get<vke_component::AudioSource>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::AudioListener>(entity))
            registry.get<vke_component::AudioListener>(entity).UnloadFromEngine();

        auto *lightManager = vke_render::Renderer::GetInstance()->lightManager.get();

        if (lightManager->HasLight<vke_render::DirectionalLight>(entity))
            lightManager->RemoveLight<vke_render::DirectionalLight>(entity);

        if (lightManager->HasLight<vke_render::PointLight>(entity))
            lightManager->RemoveLight<vke_render::PointLight>(entity);

        if (lightManager->HasLight<vke_render::SpotLight>(entity))
            lightManager->RemoveLight<vke_render::SpotLight>(entity);
    }

    void Scene::physicsUpdateCallback(void *self, void *info)
    {
        Scene &scene = *(Scene *)self;
        JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
        auto view = scene.registry.view<Transform, vke_component::RigidBody>();
        for (auto &&[entity, transform, rigidbody] : view.each())
        {
            JPH::RVec3 position;
            JPH::Quat rotation;
            interface.GetPositionAndRotation(rigidbody.bodyID, position, rotation);

            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            scene.transformSystem.SetGlobalRotation(entity, glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
        }

        auto sensorView = scene.registry.view<Transform, vke_component::Sensor>();
        for (auto &&[entity, transform, sensor] : sensorView.each())
        {
            JPH::RVec3 position;
            JPH::Quat rotation;
            interface.GetPositionAndRotation(sensor.bodyID, position, rotation);

            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            scene.transformSystem.SetGlobalRotation(entity, glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
        }

        const float deltaTime = vke_physics::PhysicsManager::GetConfig().stepTime;
        auto characterView = scene.registry.view<Transform, vke_component::CharacterController>();
        for (auto &&[entity, transform, controller] : characterView.each())
        {
            controller.Update(deltaTime);
            if (controller.character == nullptr)
                continue;

            JPH::RVec3 position = controller.character->GetPosition();
            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
        }
    }

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

    bool Scene::HasComponent(entt::entity entity, ComponentType componentType) const
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

}
