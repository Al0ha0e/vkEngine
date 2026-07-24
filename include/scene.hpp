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

namespace vke_common
{
    struct SceneData
    {
        using ScriptDataList = std::vector<vke_component::ScriptStateData>;

        std::vector<std::string> layers{"default", "editor"};
        vke_ds::id32_t maxID = 1;
        entt::registry registry;
        std::unordered_map<vke_ds::id32_t, entt::entity> idToEntity;
        std::unordered_map<entt::entity, entt::entity> parents;

        SceneData() = default;
        SceneData(const nlohmann::json &json);

        nlohmann::json ToJSON() const;

    private:
        void loadComponent(entt::entity entity, const nlohmann::json &component);
        void componentToJSON(entt::entity entity, nlohmann::json &components) const;
    };

    class Scene
    {
    public:
        vke_common::AssetHandle handle;
        std::string path;
        std::vector<std::string> layers;
        entt::registry registry;
        std::unordered_map<vke_ds::id32_t, entt::entity> idToEntity;
        bool loadedToEngine;
        SceneTransformSystem transformSystem;
        std::unordered_map<entt::entity, std::unordered_map<std::string, vke_component::ScriptStateData>> csharpScriptStates;

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;

        Scene()
            : layers({"default", "editor"}),
              registry(), idToEntity(),
              loadedToEngine(false), transformSystem(registry, idToEntity),
              idAllocator(1),
              physicsUpdateListenerID(0), initialized(true) {}

        Scene(const SceneData &data)
            : registry(), idToEntity(),
              loadedToEngine(false), transformSystem(registry, idToEntity),
              idAllocator(data.maxID),
              physicsUpdateListenerID(0), initialized(false)
        {
            init(data);
            initialized = true;
        }

        Scene(const std::string &pth, const SceneData &data)
            : path(pth),
              registry(), idToEntity(),
              loadedToEngine(false), transformSystem(registry, idToEntity),
              idAllocator(data.maxID),
              physicsUpdateListenerID(0), initialized(false)
        {
            init(data);
            initialized = true;
        }

        ~Scene() {}

        void LoadToEngine()
        {
            if (!initialized || loadedToEngine)
                return;

            auto loadView = [this]<typename T>()
            {
                auto view = registry.view<T>();
                for (auto entity : view)
                    view.template get<T>(entity).LoadToEngine();
            };

            loadView.operator()<vke_component::Camera>();
            loadView.operator()<vke_component::RenderableObject>();
            loadView.operator()<vke_component::SkeletonAnimator>();
            loadView.operator()<vke_component::UIText>();
            auto rigidBodyView = registry.view<vke_component::RigidBody>();
            for (auto entity : rigidBodyView)
                rigidBodyView.get<vke_component::RigidBody>(entity).LoadToEngine(entity);
            auto sensorView = registry.view<vke_component::Sensor>();
            for (auto entity : sensorView)
                sensorView.get<vke_component::Sensor>(entity).LoadToEngine(entity);
            loadView.operator()<vke_component::CharacterController>();
            loadView.operator()<vke_component::AudioSource>();
            loadView.operator()<vke_component::AudioListener>();

            auto directionalLightView = registry.view<vke_component::DirectionalLight>();
            for (auto entity : directionalLightView)
                directionalLightView.get<vke_component::DirectionalLight>(entity).LoadToEngine(entity);
            auto pointLightView = registry.view<vke_component::PointLight>();
            for (auto entity : pointLightView)
                pointLightView.get<vke_component::PointLight>(entity).LoadToEngine(entity);
            auto spotLightView = registry.view<vke_component::SpotLight>();
            for (auto entity : spotLightView)
                spotLightView.get<vke_component::SpotLight>(entity).LoadToEngine(entity);

            physicsUpdateListenerID = vke_physics::PhysicsManager::RegisterUpdateListener(this,
                                                                                          std::function<void(void *, void *)>(physicsUpdateCallback));

            std::vector<std::string> dataStrs;

            for (auto &[entity, states] : csharpScriptStates)
                for (auto &[className, state] : states)
                    dataStrs.push_back(state.ToCSharp(entity));

            std::vector<const char *> dataPtrs;
            for (auto &data : dataStrs)
                dataPtrs.push_back(data.c_str());
            ScriptManager::Load(dataPtrs.data(), dataPtrs.size());
            ScriptManager::Start();
            loadedToEngine = true;
        }

        void UnloadFromEngine()
        {
            if (!initialized || !loadedToEngine)
                return;

            ScriptManager::Unload();
            vke_physics::PhysicsManager::RemoveUpdateListener(physicsUpdateListenerID);
            physicsUpdateListenerID = 0;
            vke_render::Renderer::GetInstance()->lightManager->ClearLights();

            auto unloadView = [this]<typename T>()
            {
                auto view = registry.view<T>();
                for (auto entity : view)
                    view.template get<T>(entity).UnloadFromEngine();
            };

            unloadView.operator()<vke_component::AudioSource>();
            unloadView.operator()<vke_component::AudioListener>();
            unloadView.operator()<vke_component::CharacterController>();
            unloadView.operator()<vke_component::Sensor>();
            unloadView.operator()<vke_component::RigidBody>();
            unloadView.operator()<vke_component::SkeletonAnimator>();
            unloadView.operator()<vke_component::UIText>();
            unloadView.operator()<vke_component::RenderableObject>();
            unloadView.operator()<vke_component::Camera>();
            vke_render::Renderer::GetGlyphManager()->ClearGlyphs();
            loadedToEngine = false;
        }

        entt::entity GetObjectEntity(vke_ds::id32_t id) const
        {
            auto it = idToEntity.find(id);
            return it == idToEntity.end() ? entt::null : it->second;
        }

        vke_ds::id32_t GetEntityID(entt::entity entity) const
        {
            return registry.valid(entity) ? registry.get<GameObject>(entity).id : 0;
        }

        bool HasComponent(entt::entity entity, ComponentType componentType) const;

        entt::entity AddObject(std::string &name, glm::vec3 pos, glm::vec3 scl, glm::quat rot, int layer, bool isStatic)
        {
            vke_ds::id32_t id = idAllocator.Alloc();
            entt::entity entity = registry.create();
            idToEntity[id] = entity;
            registry.emplace<GameObject>(entity, id, name, layer, isStatic);
            registry.emplace<Transform>(entity, pos, scl, rot);
            return entity;
        }

        void FillData(SceneData &data) const;

        void RemoveObject(entt::entity entity)
        {
            if (!registry.valid(entity))
                return;

            std::vector<entt::entity> entitiesToBeRemoved;
            transformSystem.PrepareForRemove(entity, entitiesToBeRemoved);

            for (entt::entity ent : entitiesToBeRemoved)
            {
                vke_ds::id32_t deadID = registry.get<GameObject>(ent).id;
                idToEntity.erase(deadID);
                unloadEntityFromEngine(ent);
            }

            for (entt::entity ent : entitiesToBeRemoved)
                registry.destroy(ent);

            // TODO maybe unload from C# (or C# unload call this func)
        }

    private:
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> idAllocator;
        vke_ds::id32_t physicsUpdateListenerID;
        bool initialized;

        void init(const SceneData &data);
        void unloadEntityFromEngine(entt::entity entity);

        static void physicsUpdateCallback(void *self, void *info);
    };

    class SceneManager
    {
    public:
        std::unique_ptr<Scene> currentScene;

    private:
        static SceneManager *instance;
        SceneManager() : currentScene(nullptr) {}
        // SceneManager(std::unique_ptr<Scene> &&scene)
        //     : currentScene(std::forward<std::unique_ptr<Scene>>(scene)) {}
        ~SceneManager() {}

    public:
        static SceneManager *GetInstance()
        {
            VKE_FATAL_IF(instance == nullptr, "SceneManager not initialized!")
            return instance;
        }

        static SceneManager *Init()
        {
            instance = new SceneManager();
            return instance;
        }

        static void Dispose()
        {
            instance->unloadCurrentSceneFromEngine();
            delete instance;
            instance = nullptr;
        }

        static void SetCurrentScene(std::unique_ptr<Scene> &&scene)
        {
            instance->unloadCurrentSceneFromEngine();
            instance->currentScene = std::move(scene);
            instance->loadCurrentSceneToEngine();
        }

        static std::unique_ptr<Scene> LoadScene(const std::string &pth) // load scene data only, not load to engine
        {
            nlohmann::json json(vke_common::LoadJSON(pth));
            SceneData data(json);
            return std::make_unique<Scene>(pth, data);
        }

        static void SaveScene(const std::string &pth)
        {
            if (instance->currentScene != nullptr)
            {
                SceneData data;
                instance->currentScene->FillData(data);
                std::ofstream ofs(pth);
                ofs << data.ToJSON().dump(4);
                ofs.close();
            }
        }

    private:
        void loadCurrentSceneToEngine()
        {
            if (currentScene == nullptr)
                return;
            currentScene->LoadToEngine();
        }

        void unloadCurrentSceneFromEngine()
        {
            if (currentScene == nullptr)
                return;
            currentScene->UnloadFromEngine();
            currentScene = nullptr;
        }
    };
}

#endif
