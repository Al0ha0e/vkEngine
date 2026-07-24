#ifndef SCENE_TRANSFORM_SYSTEM_H
#define SCENE_TRANSFORM_SYSTEM_H

#include <component/transform.hpp>
#include <ds/id_allocator.hpp>
#include <entt/entity/registry.hpp>
#include <unordered_map>
#include <vector>

namespace vke_common
{
    class SceneTransformSystem
    {
    public:
        SceneTransformSystem(entt::registry &registry,
                             std::unordered_map<vke_ds::id32_t, entt::entity> &idToEntity)
            : registry(registry), idToEntity(idToEntity) {}

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
        std::unordered_map<vke_ds::id32_t, entt::entity> &idToEntity;

        void updateTransform(entt::entity entity, Transform &transform, bool first, bool updatePhysicsComponents);
    };
}

#endif
