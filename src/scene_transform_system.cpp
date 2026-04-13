#include <scene_transform_system.hpp>
#include <logger.hpp>
#include <vector>

namespace vke_common
{
    static inline glm::vec3 TransformForward(const Transform &transform)
    {
        return transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void SceneTransformSystem::dfs(entt::entity entity, Transform &transform, std::unordered_set<entt::entity> &visited)
    {
        visited.insert(entity);

        if (transform.parent != entt::null)
            transform.SetParentFixedLocal(registry.get<Transform>(transform.parent));

        for (auto child : transform.children)
        {
            VKE_FATAL_IF(visited.find(child) != visited.end(), "SCENE OBJ CONTAIN LOOP")
            Transform &childTransform = registry.get<Transform>(child);
            dfs(child, childTransform, visited);
        }
    }

    void SceneTransformSystem::InitializeHierarchy(const nlohmann::json &jsonObjs)
    {
        for (auto &jsonObj : jsonObjs)
        {
            vke_ds::id32_t id = jsonObj["id"].get<vke_ds::id32_t>();
            Transform &transform = registry.get<Transform>(idToEntity[id]);

            vke_ds::id32_t parentId = jsonObj["parent"].get<vke_ds::id32_t>();
            transform.parent = parentId ? idToEntity.at(parentId) : entt::null;

            for (auto &jsonChild : jsonObj["children"])
                transform.children.insert(idToEntity[jsonChild.get<vke_ds::id32_t>()]);
        }

        std::unordered_set<entt::entity> visited;
        for (auto &[id, entity] : idToEntity)
        {
            Transform &transform = registry.get<Transform>(entity);
            if (transform.parent == entt::null)
                dfs(entity, transform, visited);
        }
    }

    void SceneTransformSystem::PrepareForRemove(entt::entity entity, std::vector<entt::entity> &entities)
    {
        Transform &transform = registry.get<Transform>(entity);
        if (transform.parent != entt::null)
            RemoveChild(transform.parent, entity);

        entities = {entity};
        int now = 0;
        while (now < entities.size())
        {
            entt::entity deadEntity = entities[now++];
            const Transform &deadTransform = registry.get<Transform>(deadEntity);
            for (auto &child : deadTransform.children)
                entities.push_back(child);
        }
    }

    void SceneTransformSystem::RemoveChild(entt::entity entity, entt::entity childEntity)
    {
        registry.get<Transform>(entity).children.erase(childEntity);
    }

    void SceneTransformSystem::SetParent(entt::entity entity, entt::entity parentEntity)
    {
        Transform &transform = registry.get<Transform>(entity);

        if (transform.parent == parentEntity)
            return;

        if (transform.parent != entt::null)
        {
            RemoveChild(transform.parent, entity);
            transform.RemoveParent();
        }
        transform.parent = parentEntity;
        if (transform.parent == entt::null)
        {
            updateTransform(entity, transform, true);
            return;
        }

        transform.SetParent(registry.get<Transform>(transform.parent));
        registry.get<Transform>(transform.parent).children.insert(entity);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::SetLocalPosition(entt::entity entity, const glm::vec3 &position)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.SetLocalPositionWithParent(registry.get<Transform>(transform.parent).model, position)
                                       : transform.SetLocalPosition(position);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::SetGlobalPosition(entt::entity entity, const glm::vec3 &position)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.SetGlobalPositionWithParent(registry.get<Transform>(transform.parent).model, position)
                                       : transform.SetGlobalPosition(position);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::SetLocalRotation(entt::entity entity, const glm::quat &rotation)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.SetLocalRotationWithParent(registry.get<Transform>(transform.parent), rotation)
                                       : transform.SetLocalRotation(rotation);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::SetGlobalRotation(entt::entity entity, const glm::quat &rotation)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.SetGlobalRotationWithParent(registry.get<Transform>(transform.parent), rotation)
                                       : transform.SetGlobalRotation(rotation);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::SetLocalScale(entt::entity entity, const glm::vec3 &scale)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.SetLocalScaleWithParent(registry.get<Transform>(transform.parent), scale)
                                       : transform.SetLocalScale(scale);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::RotateGlobal(entt::entity entity, float det, const glm::vec3 &axis)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.RotateGlobalWithParent(registry.get<Transform>(transform.parent), det, axis)
                                       : transform.RotateGlobal(det, axis);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::RotateLocal(entt::entity entity, float det, const glm::vec3 &axis)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.RotateLocalWithParent(registry.get<Transform>(transform.parent), det, axis)
                                       : transform.RotateLocal(det, axis);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::TranslateLocal(entt::entity entity, const glm::vec3 &det)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.TranslateLocalWithParent(registry.get<Transform>(transform.parent).model, det)
                                       : transform.TranslateLocal(det);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::TranslateGlobal(entt::entity entity, const glm::vec3 &det)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.TranslateGlobalWithParent(registry.get<Transform>(transform.parent).model, det)
                                       : transform.TranslateGlobal(det);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::Scale(entt::entity entity, const glm::vec3 &scale)
    {
        Transform &transform = registry.get<Transform>(entity);
        transform.parent != entt::null ? transform.ScaleWithParent(registry.get<Transform>(transform.parent), scale)
                                       : transform.Scale(scale);
        updateTransform(entity, transform, true);
    }

    void SceneTransformSystem::updateTransform(entt::entity entity, Transform &transform, bool first)
    {
        if (!first)
            transform.UpdateWithParent(registry.get<Transform>(transform.parent));

        if (registry.all_of<vke_component::Camera>(entity))
            registry.get<vke_component::Camera>(entity).OnTransformed(transform);

        if (registry.all_of<vke_component::RigidBody>(entity))
            registry.get<vke_component::RigidBody>(entity).OnTransformed(transform);

        if (lighting.HasLight<vke_render::DirectionalLight>(entity))
        {
            auto &light = lighting.GetLight<vke_render::DirectionalLight>(entity);
            light.direction = glm::vec4(glm::normalize(TransformForward(transform)), 0.0f);
            lighting.MarkDirty();
        }

        if (lighting.HasLight<vke_render::PointLight>(entity))
        {
            auto &light = lighting.GetLight<vke_render::PointLight>(entity);
            light.positionWithRadius = glm::vec4(transform.GetGlobalPosition(), light.positionWithRadius.w);
            lighting.MarkDirty();
        }

        if (lighting.HasLight<vke_render::SpotLight>(entity))
        {
            auto &light = lighting.GetLight<vke_render::SpotLight>(entity);
            light.positionWithRadius = glm::vec4(transform.GetGlobalPosition(), light.positionWithRadius.w);
            light.direction = glm::vec4(glm::normalize(TransformForward(transform)), 0.0f);
            lighting.MarkDirty();
        }

        for (auto &child : transform.children)
        {
            auto &childTransform = registry.get<Transform>(child);
            updateTransform(child, childTransform, false);
        }
    }
}