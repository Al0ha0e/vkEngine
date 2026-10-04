#ifndef SCENE_H
#define SCENE_H

#include <entt/entity/registry.hpp>
#include <ds/id_allocator.hpp>
#include <asset/asset_manager.hpp>
#include <component.hpp>
#include <script.hpp>
#include <gameobject.hpp>
#include <component/transform.hpp>
#include <component/script.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/skeleton_animator.hpp>
#include <component/rigidbody.hpp>
#include <component/sensor.hpp>
#include <component/character_controller.hpp>
#include <component/text.hpp>
#include <component/audio_source.hpp>
#include <component/audio_listener.hpp>
#include <component/light.hpp>
#include <scene_transform_system.hpp>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <type_traits>
#include <asset/asset_ref.hpp>

namespace vke_common
{
    enum class SceneDataStage { Parsed, Expanded, Ready };

    struct PrefabReference
    {
        AssetRef<const SceneData> scene;
        bool overrideName = false;
        bool overrideStatic = false;
        bool overrideTransform = false;
    };

    struct SceneData
    {
        using ScriptDataList = std::vector<vke_component::ScriptStateData>;

        vke_ds::id32_t maxID = 1;
        entt::registry registry;
        std::unordered_map<vke_ds::id32_t, entt::entity> idToEntity;
        std::unordered_map<entt::entity, entt::entity> parents;
        SceneDataStage stage = SceneDataStage::Parsed;
        entt::entity prefabRoot = entt::null;

        SceneData() = default;
        SceneData(const nlohmann::json &json);

        static SceneResult<SceneData> FromJSON(const nlohmann::json &json);
        // Requires validated input; checks the additional prefab contract only.
        SceneResult<void> ValidatePrefab() const;
        // Expansion postcondition, checked before advancing to Expanded.
        SceneResult<void> ValidateExpanded() const;
        // Runtime prerequisites only; does not revalidate the source hierarchy.
        SceneResult<void> ValidateReady(bool requireSingleRoot = false) const;
        SceneData Clone() const;
        void CopyComponents(entt::entity source, SceneData &target, entt::entity destination) const;

        nlohmann::json ToJSON() const;

    private:
        // Full structural check at ingestion; later stages preserve these invariants.
        SceneResult<void> validateParsed() const;
        SceneResult<void> loadComponent(entt::entity entity, const nlohmann::json &component);    // component.cpp
        void componentToJSON(entt::entity entity, nlohmann::json &components) const; // component.cpp
    };

    struct InstantiateOptions
    {
        entt::entity parent = entt::null;
        std::optional<TransformData> rootTransform;
    };

    class SceneManager
    {
    private:
        static SceneManager *instance;
        vke_ds::id32_t physicsUpdateListenerID;
        std::unordered_set<entt::entity> pendingDestroy;
        bool processingDestroy = false;
        bool shuttingDown = false;
        struct InstantiateRequest
        {
            AssetHandle prefab;
            InstantiateOptions options;
        };
        std::vector<InstantiateRequest> pendingInstantiations;
        bool processingInstantiations = false;

        SceneManager()
            : transformSystem(registry), physicsUpdateListenerID(0) {}
        ~SceneManager() {}

    public:
        using EntityMap = std::unordered_map<entt::entity, entt::entity>;
        entt::registry registry;
        SceneTransformSystem transformSystem;
        std::unordered_map<entt::entity, std::unordered_map<std::string, vke_component::ScriptStateData>> csharpScriptStates;

        SceneManager(const SceneManager &) = delete;
        SceneManager &operator=(const SceneManager &) = delete;

        static SceneManager *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "SceneManager not initialized!")
            return instance;
        }

        static SceneManager *Init()
        {
            instance = new SceneManager();
            instance->physicsUpdateListenerID =
                vke_physics::PhysicsManager::RegisterUpdateListener(
                    instance, std::function<void(void *, void *)>(physicsUpdateCallback));
            return instance;
        }

        static void Dispose()
        {
            instance->dispose();
            delete instance;
            instance = nullptr;
        }

        static SceneResult<void> Instantiate(
            const SceneData &data, const InstantiateOptions &options = {});
        static SceneResult<void> RequestInstantiate(AssetHandle prefab, const InstantiateOptions &options = {});
        void ProcessInstantiationRequests();

        bool HasComponent(entt::entity entity, ComponentType componentType) const; // component.cpp

        // Synchronous, main-thread structural changes; do not call while iterating affected pools.
        // Transform is mandatory. Scripts use a separate managed lifecycle and are not supported here.
        SceneResult<void> AddComponent(entt::entity entity, ComponentType componentType);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::CameraData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::RenderableObjectData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::SkeletonAnimatorData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::RigidBodyData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::SensorData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::CharacterControllerData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::DirectionalLightData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::PointLightData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::SpotLightData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::UITextData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::AudioSourceData &data);
        SceneResult<void> AddComponent(entt::entity entity, const vke_component::AudioListenerData &data);
        SceneResult<void> RemoveComponent(entt::entity entity, ComponentType componentType);

        entt::entity CreateEntity(std::string &name, glm::vec3 pos, glm::vec3 scl, glm::quat rot, bool isStatic)
        {
            if (shuttingDown)
                return entt::null;
            entt::entity entity = registry.create();
            registry.emplace<GameObject>(entity, name, isStatic);
            registry.emplace<Transform>(entity, pos, scl, rot);
            return entity;
        }

        // Marks the entire subtree now; actual teardown happens at the next frame boundary.
        void DestroyEntity(entt::entity entity);
        void ProcessDestroyRequests();

        bool IsPendingDestroy(entt::entity entity) const { return pendingDestroy.contains(entity); }

        SceneData ExportAllEntities() const;                    // scene.cpp
        SceneData ExportEntitySubtree(entt::entity root) const; // scene.cpp

    private:
        SceneResult<void> validateComponentMutation(entt::entity entity) const;
        template <typename Component, typename Data>
        SceneResult<void> addComponent(entt::entity entity, Data data);
        void removeNativeComponent(entt::entity entity, ComponentType componentType);

        // Data resources are already resolved. Shared by scene instantiation and individual additions.
        template <typename Component, typename Data>
        void addPreparedComponent(entt::entity entity, const Data &data)
        {
            if constexpr (std::is_empty_v<Component>)
            {
                Component::LoadToEngine(entity, registry.get<Transform>(entity), data);
                registry.emplace<Component>(entity);
            }
            else if constexpr (requires(Component &component) { component.LoadToEngine(entity, registry.get<Transform>(entity), data); })
            {
                auto &component = registry.emplace<Component>(entity);
                component.LoadToEngine(entity, registry.get<Transform>(entity), data);
            }
            else
            {
                auto &component = [&]() -> Component &
                {
                    if constexpr (std::is_constructible_v<Component, Transform &, const Data &>)
                        return registry.emplace<Component>(entity, registry.get<Transform>(entity), data);
                    else
                        return registry.emplace<Component>(entity, data);
                }();
                if constexpr (requires { component.LoadToEngine(registry, entity); })
                    component.LoadToEngine(registry, entity);
                else if constexpr (requires { component.LoadToEngine(entity); })
                    component.LoadToEngine(entity);
                else
                    component.LoadToEngine();
            }
        }

        struct PreparedScript
        {
            entt::entity entity;
            std::string className;
            TypeInfoDataPtr data;
        };
        EntityMap instantiateSceneData(const SceneData &data, const InstantiateOptions &options);
        void loadScripts(const EntityMap &dataToRuntime, const std::vector<PreparedScript> &scripts);
        void unloadEntityFromEngine(entt::entity entity);
        SceneData exportEntities(const std::vector<entt::entity> &entities) const; // scene.cpp
        void dispose();

        static void physicsUpdateCallback(void *self, void *info);
    };
}

#endif
