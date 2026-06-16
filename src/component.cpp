#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/skeleton_animator.hpp>
#include <component/rigidbody.hpp>
#include <component/sensor.hpp>
#include <component/character_controller.hpp>
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

        if (registry.all_of<vke_component::SkeletonAnimator>(entity))
            registry.get<vke_component::SkeletonAnimator>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::CharacterController>(entity))
            registry.get<vke_component::CharacterController>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::Sensor>(entity))
            registry.get<vke_component::Sensor>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::RigidBody>(entity))
            registry.get<vke_component::RigidBody>(entity).UnloadFromEngine();

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
            JPH::Quat rotation = controller.character->GetRotation();
            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            scene.transformSystem.SetGlobalRotation(entity, glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
        }
    }

    void Scene::loadComponent(const vke_ds::id32_t id, const entt::entity entity,
                              const nlohmann::json &component)
    {
        Transform &transform = registry.get<Transform>(entity);

        std::string type = component["type"];
        if (type == "camera")
        {
            registry.emplace<vke_component::Camera>(entity, transform, component);
        }
        else if (type == "renderableObject")
        {
            registry.emplace<vke_component::RenderableObject>(entity, transform, component);
        }
        else if (type == "animator")
        {
            registry.emplace<vke_component::SkeletonAnimator>(entity, transform, component);
        }
        else if (type == "rigidbody")
        {
            registry.emplace<vke_component::RigidBody>(entity, transform, component);
        }
        else if (type == "sensor")
        {
            registry.emplace<vke_component::Sensor>(entity, transform, component);
        }
        else if (type == "characterController")
        {
            registry.emplace<vke_component::CharacterController>(entity, transform, component);
        }
        else if (type == "directionalLight")
        {
            auto &color = component["color"];
            float intensity = component["intensity"];
            lighting.AddLight(
                entity,
                vke_render::DirectionalLight(
                    glm::vec4(glm::normalize(TransformForward(transform)), 0.0f),
                    glm::vec4(color[0], color[1], color[2], intensity)));
        }
        else if (type == "pointLight")
        {
            auto &color = component["color"];
            float radius = component["radius"];
            float intensity = component["intensity"];
            lighting.AddLight(
                entity,
                vke_render::PointLight(
                    glm::vec4(transform.GetGlobalPosition(), radius),
                    glm::vec4(color[0], color[1], color[2], intensity)));
        }
        else if (type == "spotLight")
        {
            auto &color = component["color"];
            float radius = component["radius"];
            float intensity = component["intensity"];
            float innerCone = glm::radians(component["innerCone"].get<float>());
            float outerCone = glm::radians(component["outerCone"].get<float>());
            lighting.AddLight(
                entity,
                vke_render::SpotLight(
                    glm::vec4(transform.GetGlobalPosition(), radius),
                    glm::vec4(glm::normalize(TransformForward(transform)), 0.0f),
                    glm::vec4(color[0], color[1], color[2], intensity),
                    glm::vec4(glm::cos(innerCone), glm::cos(outerCone), 0.0f, 0.0f)));
        }
        else if (type == "script")
        {
            vke_component::ScriptState scriptState(component);
            csharpScriptStates[entity].emplace(scriptState.className, std::move(scriptState));
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
            return vke_render::Renderer::GetInstance()->lightManager->HasLight<vke_render::DirectionalLight>(entity);
        case ComponentType::PointLight:
            return vke_render::Renderer::GetInstance()->lightManager->HasLight<vke_render::PointLight>(entity);
        case ComponentType::SpotLight:
            return vke_render::Renderer::GetInstance()->lightManager->HasLight<vke_render::SpotLight>(entity);
        case ComponentType::Script:
            return csharpScriptStates.find(entity) != csharpScriptStates.end();
        default:
            return false;
        }
    }

    void Scene::componentToJSON(const vke_ds::id32_t id, nlohmann::json &components, const vke_render::SceneLightData &lightData)
    {
        const entt::entity entity = idToEntity[id];

        if (registry.all_of<vke_component::Camera>(entity))
            components.push_back(registry.get<vke_component::Camera>(entity).ToJSON());

        if (registry.all_of<vke_component::RenderableObject>(entity))
            components.push_back(registry.get<vke_component::RenderableObject>(entity).ToJSON());

        if (registry.all_of<vke_component::SkeletonAnimator>(entity))
            components.push_back(registry.get<vke_component::SkeletonAnimator>(entity).ToJSON());

        if (registry.all_of<vke_component::RigidBody>(entity))
            components.push_back(registry.get<vke_component::RigidBody>(entity).ToJSON());

        if (registry.all_of<vke_component::Sensor>(entity))
            components.push_back(registry.get<vke_component::Sensor>(entity).ToJSON());

        if (registry.all_of<vke_component::CharacterController>(entity))
            components.push_back(registry.get<vke_component::CharacterController>(entity).ToJSON());

        if (lightData.HasLight<vke_render::DirectionalLight>(entity))
            components.push_back(lightData.GetLight<vke_render::DirectionalLight>(entity).ToJSON());

        if (lightData.HasLight<vke_render::PointLight>(entity))
            components.push_back(lightData.GetLight<vke_render::PointLight>(entity).ToJSON());

        if (lightData.HasLight<vke_render::SpotLight>(entity))
            components.push_back(lightData.GetLight<vke_render::SpotLight>(entity).ToJSON());

        auto scriptIt = csharpScriptStates.find(entity);
        if (scriptIt != csharpScriptStates.end())
        {
            for (const auto &[className, scriptState] : scriptIt->second)
                components.push_back(scriptState.ToJSON());
        }
    }

}
