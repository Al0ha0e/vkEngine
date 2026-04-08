#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/skeleton_animator.hpp>
#include <component/rigidbody.hpp>
#include <component/light.hpp>
#include <component/script.hpp>
#include <scene.hpp>

namespace vke_common
{
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

            scene.SetLocalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            scene.SetLocalRotation(entity, glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
            // TODO Set Global
        }
    }

    void Scene::loadComponent(const vke_ds::id32_t id, const nlohmann::json &component)
    {
        const entt::entity entity = idToEntity[id];
        Transform &transform = registry.get<Transform>(entity);

        std::string type = component["type"];
        if (type == "camera")
            registry.emplace<vke_component::Camera>(entity, transform, component);
        else if (type == "renderableObject")
            registry.emplace<vke_component::RenderableObject>(entity, transform, component);
        else if (type == "animator")
            registry.emplace<vke_component::SkeletonAnimator>(entity, transform, component);
        else if (type == "rigidbody")
            registry.emplace<vke_component::RigidBody>(entity, transform, component);
        else if (type == "directionalLight")
            registry.emplace<vke_component::DirectionalLightComponent>(entity, transform, component);
        else if (type == "pointLight")
            registry.emplace<vke_component::PointLightComponent>(entity, transform, component);
        else if (type == "spotLight")
            registry.emplace<vke_component::SpotLightComponent>(entity, transform, component);
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

        if (registry.all_of<vke_component::DirectionalLightComponent>(entity))
            components.push_back(registry.get<vke_component::DirectionalLightComponent>(entity).ToJSON());

        if (registry.all_of<vke_component::PointLightComponent>(entity))
            components.push_back(registry.get<vke_component::PointLightComponent>(entity).ToJSON());

        if (registry.all_of<vke_component::SpotLightComponent>(entity))
            components.push_back(registry.get<vke_component::SpotLightComponent>(entity).ToJSON());

        // auto scriptIt = csharpScripts.find(id);
        // if (scriptIt != csharpScripts.end())
        // {
        //     for (const auto &[className, scriptState] : scriptIt->second)
        //         components.push_back(scriptState.ToJSON());
        // }
    }

    void Scene::updateTransform(entt::entity entity, Transform &transform, bool first)
    {
        if (!first)
            transform.UpdateWithParent(registry.get<Transform>(transform.parent));

        if (vke_component::Camera *camera = registry.try_get<vke_component::Camera>(entity))
            camera->OnTransformed(transform);

        if (vke_component::DirectionalLightComponent *directionalLight = registry.try_get<vke_component::DirectionalLightComponent>(entity))
            directionalLight->OnTransformed(transform);

        if (vke_component::PointLightComponent *pointLight = registry.try_get<vke_component::PointLightComponent>(entity))
            pointLight->OnTransformed(transform);

        if (vke_component::SpotLightComponent *spotLight = registry.try_get<vke_component::SpotLightComponent>(entity))
            spotLight->OnTransformed(transform);

        for (auto &child : transform.children)
        {
            auto childTransform = registry.get<Transform>(child);
            updateTransform((entt::entity)child, childTransform, false);
        }
    }
}
