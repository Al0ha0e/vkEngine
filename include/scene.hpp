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

namespace vke_common
{
    struct SceneData
    {
        using ScriptDataList = std::vector<vke_component::ScriptStateData>;

        vke_ds::id32_t maxID = 1;
        entt::registry registry;
        std::unordered_map<vke_ds::id32_t, entt::entity> idToEntity;
        std::unordered_map<entt::entity, entt::entity> parents;

        SceneData() = default;
        SceneData(const nlohmann::json &json);

        nlohmann::json ToJSON() const;

    private:
        void loadComponent(entt::entity entity, const nlohmann::json &component);    // component.cpp
        void componentToJSON(entt::entity entity, nlohmann::json &components) const; // component.cpp
    };

    class SceneManager
    {
    private:
        static SceneManager *instance;
        vke_ds::id32_t physicsUpdateListenerID;

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

        static EntityMap LoadSceneData(const SceneData &data)
        {
            EntityMap dataToRuntime = instance->instantiateSceneData(data);
            instance->loadEntitiesToEngine(dataToRuntime);
            return dataToRuntime;
        }

        static void UnloadSceneData(const EntityMap &dataToRuntime)
        {
            instance->unloadSceneData(dataToRuntime);
        }

        bool HasComponent(entt::entity entity, ComponentType componentType) const; // component.cpp

        entt::entity AddObject(std::string &name, glm::vec3 pos, glm::vec3 scl, glm::quat rot, bool isStatic)
        {
            entt::entity entity = registry.create();
            registry.emplace<GameObject>(entity, name, isStatic);
            registry.emplace<Transform>(entity, pos, scl, rot);
            return entity;
        }

        void RemoveObject(entt::entity entity)
        {
            if (!registry.valid(entity))
                return;

            std::vector<entt::entity> entitiesToBeRemoved;
            transformSystem.PrepareForRemove(entity, entitiesToBeRemoved);

            for (entt::entity ent : entitiesToBeRemoved)
                unloadEntityFromEngine(ent);

            for (entt::entity ent : entitiesToBeRemoved)
                registry.destroy(ent);

            // TODO maybe unload from C# (or C# unload call this func)
        }

        SceneData ExportAllEntities() const;                    // scene.cpp
        SceneData ExportEntitySubtree(entt::entity root) const; // scene.cpp

    private:
        EntityMap instantiateSceneData(const SceneData &data);
        void unloadSceneData(const EntityMap &dataToRuntime);
        void loadEntitiesToEngine(const EntityMap &dataToRuntime);
        void unloadEntityFromEngine(entt::entity entity);
        SceneData exportEntities(const std::vector<entt::entity> &entities) const; // scene.cpp
        void dispose();

        static void physicsUpdateCallback(void *self, void *info);
    };
}

#endif
