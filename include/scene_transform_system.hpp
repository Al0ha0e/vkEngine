#ifndef SCENE_TRANSFORM_SYSTEM_H
#define SCENE_TRANSFORM_SYSTEM_H

#include <component/transform.hpp>
#include <entt/entity/registry.hpp>
#include <unordered_set>
#include <vector>

namespace vke_common
{
    class SceneTransformSystem
    {
    public:
        SceneTransformSystem(entt::registry &registry)
            : registry(registry) {}

        void CollectEntitySubtree(std::vector<entt::entity> &entities) const;
        void CollectEntitiesSubtree(std::unordered_set<entt::entity> &entitySet, std::vector<entt::entity> &entities) const;
        void PrepareForRemove(entt::entity entity, std::vector<entt::entity> &entities);
        void RemoveChild(entt::entity entity, entt::entity childEntity);
        void SetParent(entt::entity entity, entt::entity parentEntity, bool updatePhysicsComponents = false);
        void SetGlobalPosition(entt::entity entity, const glm::vec3 &position, bool updatePhysicsComponents = false);
        void SetGlobalRotation(entt::entity entity, const glm::quat &rotation, bool updatePhysicsComponents = false);
        void SetLocalPosition(entt::entity entity, const glm::vec3 &position, bool updatePhysicsComponents = false);
        void SetLocalRotation(entt::entity entity, const glm::quat &rotation, bool updatePhysicsComponents = false);
        void SetLocalScale(entt::entity entity, const glm::vec3 &scale, bool updatePhysicsComponents = false);
        void RotateGlobal(entt::entity entity, float det, const glm::vec3 &axis, bool updatePhysicsComponents = false);
        void RotateLocal(entt::entity entity, float det, const glm::vec3 &axis, bool updatePhysicsComponents = false);
        void TranslateLocal(entt::entity entity, const glm::vec3 &det, bool updatePhysicsComponents = false);
        void TranslateGlobal(entt::entity entity, const glm::vec3 &det, bool updatePhysicsComponents = false);
        void Scale(entt::entity entity, const glm::vec3 &scale, bool updatePhysicsComponents = false);

    private:
        entt::registry &registry;

        void updateTransform(entt::entity entity, Transform &transform, bool first, bool updatePhysicsComponents);
    };
}

#endif
