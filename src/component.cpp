#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/skeleton_animator.hpp>
#include <component/rigidbody.hpp>
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

        if (registry.all_of<vke_component::RigidBody>(entity))
            registry.get<vke_component::RigidBody>(entity).UnloadFromEngine();

        if (lighting.HasLight<vke_render::DirectionalLight>(entity))
            lighting.RemoveLight<vke_render::DirectionalLight>(entity);

        if (lighting.HasLight<vke_render::PointLight>(entity))
            lighting.RemoveLight<vke_render::PointLight>(entity);

        if (lighting.HasLight<vke_render::SpotLight>(entity))
            lighting.RemoveLight<vke_render::SpotLight>(entity);
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
            JPH::RVec3 cpos;
            cpos = interface.GetCenterOfMassPosition(rigidbody.bodyID);

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
        else if (type == "directionalLight")
        {
            auto &color = component["color"];
            float intensity = component["intensity"];
            lighting.AddLight<vke_render::DirectionalLight>(
                entity,
                glm::vec4(glm::normalize(TransformForward(transform)), 0.0f),
                glm::vec4(color[0], color[1], color[2], intensity));
        }
        else if (type == "pointLight")
        {
            auto &color = component["color"];
            float radius = component["radius"];
            float intensity = component["intensity"];
            lighting.AddLight<vke_render::PointLight>(
                entity,
                glm::vec4(transform.GetGlobalPosition(), radius),
                glm::vec4(color[0], color[1], color[2], intensity));
        }
        else if (type == "spotLight")
        {
            auto &color = component["color"];
            float radius = component["radius"];
            float intensity = component["intensity"];
            float innerCone = glm::radians(component["innerCone"].get<float>());
            float outerCone = glm::radians(component["outerCone"].get<float>());
            lighting.AddLight<vke_render::SpotLight>(
                entity,
                glm::vec4(transform.GetGlobalPosition(), radius),
                glm::vec4(glm::normalize(TransformForward(transform)), 0.0f),
                glm::vec4(color[0], color[1], color[2], intensity),
                glm::vec4(glm::cos(innerCone), glm::cos(outerCone), 0.0f, 0.0f));
        }
        else if (type == "script")
        {
            csharpScriptStates.push_back(component["data"].dump());
            // vke_component::ScriptState scriptState(id, component);
            // csharpScripts[id].emplace(scriptState.className, std::move(scriptState));
        }
    }

    void Scene::componentToJSON(const vke_ds::id32_t id, nlohmann::json &components)
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

        if (lighting.HasLight<vke_render::DirectionalLight>(entity))
            components.push_back(lighting.GetLight<vke_render::DirectionalLight>(entity).ToJSON());

        if (lighting.HasLight<vke_render::PointLight>(entity))
            components.push_back(lighting.GetLight<vke_render::PointLight>(entity).ToJSON());

        if (lighting.HasLight<vke_render::SpotLight>(entity))
            components.push_back(lighting.GetLight<vke_render::SpotLight>(entity).ToJSON());

        // auto scriptIt = csharpScripts.find(id);
        // if (scriptIt != csharpScripts.end())
        // {
        //     for (const auto &[className, scriptState] : scriptIt->second)
        //         components.push_back(scriptState.ToJSON());
        // }
    }

}