#ifndef SCENE_H
#define SCENE_H

#include <entt/entity/registry.hpp>
#include <ds/id_allocator.hpp>
#include <asset.hpp>
#include <script.hpp>
#include <gameobject.hpp>
#include <component/transform.hpp>
#include <component/script.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>
#include <component/skeleton_animator.hpp>
#include <component/rigidbody.hpp>
#include <unordered_map>
#include <unordered_set>

namespace vke_common
{
    class Scene
    {
    public:
        vke_common::AssetHandle handle;
        std::string path;
        std::vector<std::string> layers;

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;

        Scene()
            : idAllocator(1), layers({"default", "editor"}),
              physicsUpdateListenerID(0), initialized(true), loadedToEngine(false) {}

        Scene(const nlohmann::json &json)
            : idAllocator(json["maxid"]),
              physicsUpdateListenerID(0), initialized(false), loadedToEngine(false)
        {
            init(json);
            initialized = true;
        }

        Scene(const std::string &pth, const nlohmann::json &json)
            : path(pth), idAllocator(json["maxid"]),
              physicsUpdateListenerID(0), initialized(false), loadedToEngine(false)
        {
            init(json);
            initialized = true;
        }

        ~Scene() {}

        nlohmann::json ToJSON();

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
            loadView.operator()<vke_component::RigidBody>();
            vke_render::Renderer::GetInstance()->lightManager->BindSceneLighting(&lighting);

            physicsUpdateListenerID = vke_physics::PhysicsManager::RegisterUpdateListener(this,
                                                                                          std::function<void(void *, void *)>(physicsUpdateCallback));

            std::vector<const char *> dataPtrs;
            for (auto &data : csharpScriptStates)
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
            vke_render::Renderer::GetInstance()->lightManager->BindSceneLighting(nullptr);

            auto unloadView = [this]<typename T>()
            {
                auto view = registry.view<T>();
                for (auto entity : view)
                    view.template get<T>(entity).UnloadFromEngine();
            };

            unloadView.operator()<vke_component::RigidBody>();
            unloadView.operator()<vke_component::SkeletonAnimator>();
            unloadView.operator()<vke_component::RenderableObject>();
            unloadView.operator()<vke_component::Camera>();
            loadedToEngine = false;
        }

        // void AddObject(std::unique_ptr<GameObject> &&object)
        // {
        //     vke_ds::id32_t id = idAllocator.Alloc();
        //     object->id = id;
        //     entt::entity entity = registry.create();
        //     idToEntity[id] = entity;
        //     gameObjects[id] = std::move(object);
        // }

        entt::entity GetObjectEntity(vke_ds::id32_t id) const
        {
            auto it = idToEntity.find(id);
            return it == idToEntity.end() ? entt::null : it->second;
        }

        vke_ds::id32_t GetEntityID(entt::entity entity) const
        {
            return registry.valid(entity) ? registry.get<GameObject>(entity).id : 0;
        }

        void RemoveObject(entt::entity entity)
        {
            if (!registry.valid(entity))
                return;

            Transform &transform = registry.get<Transform>(entity);
            if (transform.parent != entt::null)
                RemoveChild(transform.parent, entity);

            std::vector<entt::entity> entitiesToBeRemoved = {entity};
            int now = 0;
            while (now < entitiesToBeRemoved.size())
            {
                entt::entity deadEntity = entitiesToBeRemoved[now++];
                vke_ds::id32_t deadID = registry.get<GameObject>(deadEntity).id;
                idToEntity.erase(deadID);
                Transform &deadTransform = registry.get<Transform>(deadEntity);
                for (auto &child : deadTransform.children)
                    entitiesToBeRemoved.push_back(child);
            }

            for (entt::entity ent : entitiesToBeRemoved)
                unloadEntityFromEngine(ent);

            for (entt::entity ent : entitiesToBeRemoved)
                registry.destroy(ent); // remove all component in registry
        }

        void RemoveChild(entt::entity entity, entt::entity childEntity)
        {
            registry.get<Transform>(entity).children.erase(childEntity);
        }

        void SetParent(entt::entity entity, entt::entity parentEntity)
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

        void SetLocalPosition(const entt::entity entity, const glm::vec3 &position)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.SetLocalPositionWithParent(registry.get<Transform>(transform.parent).model, position)
                                           : transform.SetLocalPosition(position);
            updateTransform(entity, transform, true);
        }

        void SetLocalRotation(const entt::entity entity, const glm::quat &rotation)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.SetLocalRotationWithParent(registry.get<Transform>(transform.parent), rotation)
                                           : transform.SetLocalRotation(rotation);
            updateTransform(entity, transform, true);
        }

        void SetLocalScale(const entt::entity entity, const glm::vec3 &scale)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.SetLocalScaleWithParent(registry.get<Transform>(transform.parent), scale)
                                           : transform.SetLocalScale(scale);
            updateTransform(entity, transform, true);
        }

        void RotateGlobal(const entt::entity entity, const float det, const glm::vec3 &axis)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.RotateGlobalWithParent(registry.get<Transform>(transform.parent), det, axis)
                                           : transform.RotateGlobal(det, axis);
            updateTransform(entity, transform, true);
        }

        void RotateLocal(const entt::entity entity, const float det, const glm::vec3 &axis)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.RotateLocalWithParent(registry.get<Transform>(transform.parent), det, axis)
                                           : transform.RotateLocal(det, axis);
            updateTransform(entity, transform, true);
        }

        void TranslateLocal(const entt::entity entity, const glm::vec3 &det)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.TranslateLocalWithParent(registry.get<Transform>(transform.parent).model, det)
                                           : transform.TranslateLocal(det);
            updateTransform(entity, transform, true);
        }

        void TranslateGlobal(const entt::entity entity, const glm::vec3 &det)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.TranslateGlobalWithParent(registry.get<Transform>(transform.parent).model, det)
                                           : transform.TranslateGlobal(det);
            updateTransform(entity, transform, true);
        }

        void Scale(const entt::entity entity, const glm::vec3 &scale)
        {
            Transform &transform = registry.get<Transform>(entity);
            transform.parent != entt::null ? transform.ScaleWithParent(registry.get<Transform>(transform.parent), scale)
                                           : transform.Scale(scale);
            updateTransform(entity, transform, true);
        }

        entt::registry registry;
        std::unordered_map<vke_ds::id32_t, entt::entity> idToEntity;
        vke_render::SceneLighting lighting;
        // std::unordered_map<vke_ds::id32_t, std::unordered_multimap<std::string, vke_component::ScriptState>> csharpScripts;
        std::vector<std::string> csharpScriptStates;

    private:
        vke_ds::NaiveIDAllocator<vke_ds::id32_t> idAllocator;
        vke_ds::id32_t physicsUpdateListenerID;
        bool initialized;
        bool loadedToEngine;

        void dfs(entt::entity entity, Transform &transform, std::unordered_set<entt::entity> &visited);
        void init(const nlohmann::json &json);
        void loadComponent(const vke_ds::id32_t id, const entt::entity entity,
                           const nlohmann::json &component);
        void componentToJSON(const vke_ds::id32_t id, nlohmann::json &json);
        void unloadEntityFromEngine(entt::entity entity);
        void updateTransform(entt::entity entity, Transform &transform, bool first);

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
            nlohmann::json json(vke_common::AssetManager::LoadJSON(pth));
            return std::make_unique<Scene>(pth, json);
        }

        static void SaveScene(const std::string &pth)
        {
            if (instance->currentScene != nullptr)
            {
                std::ofstream ofs(pth);
                ofs << instance->currentScene->ToJSON().dump(4);
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
