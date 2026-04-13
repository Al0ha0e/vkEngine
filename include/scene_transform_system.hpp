#ifndef SCENE_TRANSFORM_SYSTEM_H
#define SCENE_TRANSFORM_SYSTEM_H

#include <component/transform.hpp>
#include <component/camera.hpp>
#include <component/rigidbody.hpp>
#include <render/light.hpp>
#include <entt/entity/registry.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vke_common
{
    class SceneTransformSystem
    {
    public:
        SceneTransformSystem(entt::registry &registry,
                             std::unordered_map<vke_ds::id32_t, entt::entity> &idToEntity,
                             vke_render::SceneLighting &lighting)
            : registry(registry), idToEntity(idToEntity), lighting(lighting) {}

        void InitializeHierarchy(const nlohmann::json &jsonObjs);
        void PrepareForRemove(entt::entity entity, std::vector<entt::entity> &entities);
        void RemoveChild(entt::entity entity, entt::entity childEntity);
        void SetParent(entt::entity entity, entt::entity parentEntity);
        void SetGlobalPosition(entt::entity entity, const glm::vec3 &position);
        void SetGlobalRotation(entt::entity entity, const glm::quat &rotation);
        void SetLocalPosition(entt::entity entity, const glm::vec3 &position);
        void SetLocalRotation(entt::entity entity, const glm::quat &rotation);
        void SetLocalScale(entt::entity entity, const glm::vec3 &scale);
        void RotateGlobal(entt::entity entity, float det, const glm::vec3 &axis);
        void RotateLocal(entt::entity entity, float det, const glm::vec3 &axis);
        void TranslateLocal(entt::entity entity, const glm::vec3 &det);
        void TranslateGlobal(entt::entity entity, const glm::vec3 &det);
        void Scale(entt::entity entity, const glm::vec3 &scale);

    private:
        entt::registry &registry;
        std::unordered_map<vke_ds::id32_t, entt::entity> &idToEntity;
        vke_render::SceneLighting &lighting;

        void dfs(entt::entity entity, Transform &transform, std::unordered_set<entt::entity> &visited);
        void updateTransform(entt::entity entity, Transform &transform, bool first);
    };
}

#endif