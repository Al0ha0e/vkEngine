#include <scene.hpp>
#include <unordered_set>

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace vke_common
{
    SceneManager *SceneManager::instance = nullptr;

    SceneResult<void> SceneManager::RequestInstantiate(AssetHandle prefab, const InstantiateOptions &options)
    {
        if (!instance || instance->shuttingDown)
            return std::unexpected("SceneManager is unavailable");
        if (prefab == 0)
            return std::unexpected("prefab handle is empty");
        if (options.parent != entt::null && (!instance->registry.valid(options.parent) ||
                                             !instance->registry.all_of<Transform>(options.parent) || instance->IsPendingDestroy(options.parent)))
            return std::unexpected("invalid or pending-destroy parent");
        if (options.rootTransform)
        {
            const auto &t = *options.rootTransform;
            for (int i = 0; i < 3; ++i)
                if (!std::isfinite(t.localPosition[i]) || !std::isfinite(t.localScale[i]))
                    return std::unexpected("non-finite spawn transform");
            const float length = glm::dot(t.localRotation, t.localRotation);
            if (!std::isfinite(length) || length <= 0)
                return std::unexpected("invalid spawn rotation");
        }
        instance->pendingInstantiations.push_back({prefab, options});
        if (auto &transform = instance->pendingInstantiations.back().options.rootTransform)
            transform->localRotation = glm::normalize(transform->localRotation);
        return {};
    }

    void SceneManager::ProcessInstantiationRequests()
    {
        if (shuttingDown || processingInstantiations)
            return;
        processingInstantiations = true;
        auto requests = std::move(pendingInstantiations);
        pendingInstantiations.clear();
        // Requests issued by Start stay in the next batch.
        for (const auto &request : requests)
        {
            auto result = [&]() -> SceneResult<void>
            {
                if (request.options.parent != entt::null &&
                    (!registry.valid(request.options.parent) || IsPendingDestroy(request.options.parent)))
                    return std::unexpected("spawn parent was destroyed");
                auto asset = AssetManager::LoadSceneData(request.prefab);
                if (!asset)
                    return std::unexpected(asset.error());
                auto data = (*asset)->Clone();
                if (auto prepared = AssetManager::PrepareSceneData(data); !prepared)
                    return prepared;
                return Instantiate(data, request.options);
            }();
            if (!result)
                VKE_LOG_ERROR("Prefab {} instantiation failed: {}", request.prefab, result.error());
        }
        processingInstantiations = false;
    }

    SceneResult<void> SceneManager::Instantiate(const SceneData &data, const InstantiateOptions &options)
    {
        if (!instance || instance->shuttingDown)
            return std::unexpected("SceneManager is unavailable");
        if (auto valid = data.ValidateReady(options.rootTransform.has_value()); !valid)
            return std::unexpected(valid.error());
        if (options.parent != entt::null && (!instance->registry.valid(options.parent) ||
                                             !instance->registry.all_of<Transform>(options.parent) || instance->IsPendingDestroy(options.parent)))
            return std::unexpected("invalid or pending-destroy parent");
        std::vector<PreparedScript> scripts;
        for (const auto entity : data.registry.view<const SceneData::ScriptDataList>())
        {
            for (const auto &script : data.registry.get<SceneData::ScriptDataList>(entity))
            {
                auto encoded = script.PrepareForInstantiation();
                if (!encoded)
                    return std::unexpected(encoded.error());
                scripts.push_back({entity, script.className, std::move(*encoded)});
            }
        }
        const auto dataToRuntime = instance->instantiateSceneData(data, options);
        instance->loadEntitiesToEngine(dataToRuntime, scripts);
        return {};
    }

    SceneData SceneManager::ExportAllEntities() const
    {
        auto view = registry.view<const GameObject>();
        std::vector<entt::entity> entities;
        entities.reserve(view.size_hint());
        for (const entt::entity entity : view)
            entities.push_back(entity);
        return exportEntities(entities);
    }

    SceneData SceneManager::ExportEntitySubtree(entt::entity root) const
    {
        std::vector<entt::entity> entities{root};
        transformSystem.CollectEntitySubtree(entities);
        auto data = exportEntities(entities);
        for (const auto &[entity, parent] : data.parents)
            if (parent == entt::null)
                data.prefabRoot = entity;
        return data;
    }

    SceneData SceneManager::exportEntities(const std::vector<entt::entity> &entities) const
    {
        SceneData data;

        std::unordered_map<entt::entity, entt::entity> runtimeToData;
        runtimeToData.reserve(entities.size());
        vke_ds::id32_t nextID = 1;

        for (const entt::entity runtimeEntity : entities)
        {
            if (!registry.valid(runtimeEntity) ||
                runtimeToData.contains(runtimeEntity))
                continue;

            const GameObject &object = registry.get<GameObject>(runtimeEntity);
            entt::entity dataEntity = data.registry.create();
            data.idToEntity[nextID++] = dataEntity;
            runtimeToData[runtimeEntity] = dataEntity;

            std::string name = object.name;
            data.registry.emplace<GameObject>(
                dataEntity, name, object.isStatic);
            const Transform &transform = registry.get<Transform>(runtimeEntity);
            auto &transformData = data.registry.emplace<TransformData>(dataEntity);
            transform.FillData(transformData);

            auto fillComponentData = [&]<typename Component, typename Data>()
            {
                if (!registry.all_of<Component>(runtimeEntity))
                    return;
                auto &componentData = data.registry.emplace<Data>(dataEntity);
                registry.get<Component>(runtimeEntity).FillData(componentData);
            };

            fillComponentData.operator()<vke_component::Camera, vke_component::CameraData>();
            fillComponentData.operator()<vke_component::RenderableObject, vke_component::RenderableObjectData>();
            fillComponentData.operator()<vke_component::UIText, vke_component::UITextData>();
            fillComponentData.operator()<vke_component::SkeletonAnimator, vke_component::SkeletonAnimatorData>();
            fillComponentData.operator()<vke_component::RigidBody, vke_component::RigidBodyData>();
            fillComponentData.operator()<vke_component::Sensor, vke_component::SensorData>();
            fillComponentData.operator()<vke_component::CharacterController, vke_component::CharacterControllerData>();
            fillComponentData.operator()<vke_component::AudioSource, vke_component::AudioSourceData>();
            fillComponentData.operator()<vke_component::AudioListener, vke_component::AudioListenerData>();
            fillComponentData.operator()<vke_component::DirectionalLight, vke_component::DirectionalLightData>();
            fillComponentData.operator()<vke_component::PointLight, vke_component::PointLightData>();
            fillComponentData.operator()<vke_component::SpotLight, vke_component::SpotLightData>();

            auto scriptIt = csharpScriptStates.find(runtimeEntity);
            if (scriptIt != csharpScriptStates.end())
            {
                SceneData::ScriptDataList scripts;
                scripts.reserve(scriptIt->second.size());
                for (const auto &[className, state] : scriptIt->second)
                    scripts.push_back(state);
                data.registry.emplace<SceneData::ScriptDataList>(
                    dataEntity, std::move(scripts));
            }
        }

        for (const auto &[runtimeEntity, dataEntity] : runtimeToData)
        {
            const entt::entity runtimeParent = registry.get<Transform>(runtimeEntity).parent;
            auto parentIt = runtimeToData.find(runtimeParent);
            data.parents[dataEntity] =
                parentIt == runtimeToData.end() ? entt::null : parentIt->second;
        }

        data.maxID = nextID;
        data.stage = SceneDataStage::Ready;

        return data;
    }

    SceneManager::EntityMap SceneManager::instantiateSceneData(const SceneData &data, const InstantiateOptions &options)
    {
        EntityMap dataToRuntime;
        dataToRuntime.reserve(data.idToEntity.size());

        /////////////////////////////////// alloc engine entity ///////////////////////////////////////////

        for (const auto &entry : data.idToEntity)
        {
            const entt::entity dataEntity = entry.second;
            const GameObject &object = data.registry.get<GameObject>(dataEntity);
            entt::entity runtimeEntity = registry.create();
            dataToRuntime[dataEntity] = runtimeEntity;

            std::string name = object.name;
            registry.emplace<GameObject>(
                runtimeEntity, name, object.isStatic);
            auto parent = data.parents.find(dataEntity);
            const bool root = parent == data.parents.end() || parent->second == entt::null;
            registry.emplace<Transform>(runtimeEntity, root && options.rootTransform
                                                           ? *options.rootTransform
                                                           : data.registry.get<TransformData>(dataEntity));
        }

        ///////////////////////////// construct parent relation & update transform /////////////////////////

        for (const auto &[dataEntity, runtimeEntity] : dataToRuntime)
        {
            Transform &transform = registry.get<Transform>(runtimeEntity);
            auto parentIt = data.parents.find(dataEntity);
            transform.parent = parentIt == data.parents.end() || parentIt->second == entt::null
                                   ? options.parent
                                   : dataToRuntime.at(parentIt->second);
            if (transform.parent != entt::null)
                registry.get<Transform>(transform.parent).children.insert(runtimeEntity);
        }

        std::unordered_set<entt::entity> visited;
        std::function<void(entt::entity)> updateHierarchy = [&](entt::entity entity)
        {
            if (!visited.insert(entity).second)
                return;
            Transform &transform = registry.get<Transform>(entity);
            if (transform.parent != entt::null)
                transform.SetParentFixedLocal(registry.get<Transform>(transform.parent));
            for (entt::entity child : transform.children)
                updateHierarchy(child);
        };
        for (const auto &[dataEntity, runtimeEntity] : dataToRuntime)
            if (registry.get<Transform>(runtimeEntity).parent == options.parent)
                updateHierarchy(runtimeEntity);

        /////////////////////////////////// construct components //////////////////////////////////////////

        auto construct = [&]<typename Data, typename Component>()
        {
            auto view = data.registry.view<const Data>();
            for (entt::entity dataEntity : view)
            {
                entt::entity runtimeEntity = dataToRuntime.at(dataEntity);
                registry.emplace<Component>(
                    runtimeEntity,
                    registry.get<Transform>(runtimeEntity),
                    data.registry.get<Data>(dataEntity));
            }
        };
        auto constructWithoutTransform = [&]<typename Data, typename Component>()
        {
            auto view = data.registry.view<const Data>();
            for (entt::entity dataEntity : view)
            {
                registry.emplace<Component>(
                    dataToRuntime.at(dataEntity),
                    data.registry.get<Data>(dataEntity));
            }
        };

        construct.operator()<vke_component::CameraData, vke_component::Camera>();
        construct.operator()<vke_component::RenderableObjectData, vke_component::RenderableObject>();
        construct.operator()<vke_component::UITextData, vke_component::UIText>();
        construct.operator()<vke_component::SkeletonAnimatorData, vke_component::SkeletonAnimator>();
        construct.operator()<vke_component::RigidBodyData, vke_component::RigidBody>();
        construct.operator()<vke_component::SensorData, vke_component::Sensor>();
        construct.operator()<vke_component::CharacterControllerData, vke_component::CharacterController>();
        constructWithoutTransform.operator()<vke_component::AudioSourceData, vke_component::AudioSource>();
        constructWithoutTransform.operator()<vke_component::AudioListenerData, vke_component::AudioListener>();
        construct.operator()<vke_component::DirectionalLightData, vke_component::DirectionalLight>();
        construct.operator()<vke_component::PointLightData, vke_component::PointLight>();
        construct.operator()<vke_component::SpotLightData, vke_component::SpotLight>();

        auto scriptView = data.registry.view<const SceneData::ScriptDataList>();
        for (entt::entity dataEntity : scriptView)
        {
            const entt::entity runtimeEntity = dataToRuntime.at(dataEntity);
            for (const vke_component::ScriptStateData &scriptData :
                 data.registry.get<SceneData::ScriptDataList>(dataEntity))
                csharpScriptStates[runtimeEntity].emplace(
                    scriptData.className, scriptData);
        }

        return dataToRuntime;
    }

    void SceneManager::DestroyEntity(entt::entity entity)
    {
        // Only collect and mark the subtree; keep its hierarchy until actual deletion.
        std::vector<entt::entity> entities{entity};
        for (size_t i = 0; i < entities.size(); ++i)
        {
            const auto current = entities[i];
            if (!registry.valid(current) || !pendingDestroy.insert(current).second)
                continue;
            if (const auto *transform = registry.try_get<Transform>(current))
                entities.insert(entities.end(), transform->children.begin(), transform->children.end());
        }
    }

    void SceneManager::ProcessDestroyRequests()
    {
        if (processingDestroy)
            return;
        processingDestroy = true;
        // Keep pending markers until reclamation; callback additions stay outside this snapshot.
        const std::vector<entt::entity> current(pendingDestroy.begin(), pendingDestroy.end());

        // Keep every native entity and hierarchy link alive throughout all Unload callbacks.
        if (!current.empty())
            ScriptManager::UnloadEntities(current);

        for (const entt::entity entity : current)
        {
            if (registry.valid(entity))
            {
                unloadEntityFromEngine(entity);
                csharpScriptStates.erase(entity);
                // Remove the parent's reference only when reclaiming this entity.
                if (const auto *transform = registry.try_get<Transform>(entity);
                    transform && registry.valid(transform->parent))
                {
                    if (auto *parent = registry.try_get<Transform>(transform->parent))
                        parent->children.erase(entity);
                }
                registry.destroy(entity);
            }
            pendingDestroy.erase(entity);
        }
        processingDestroy = false;
    }

    void SceneManager::loadEntitiesToEngine(const EntityMap &dataToRuntime, const std::vector<PreparedScript> &scripts)
    {
        std::vector<entt::entity> runtimeEntities;
        runtimeEntities.reserve(dataToRuntime.size());
        for (const auto &[dataEntity, runtimeEntity] : dataToRuntime)
            runtimeEntities.push_back(runtimeEntity);

        auto loadView = [this, &runtimeEntities]<typename T>()
        {
            for (const entt::entity runtimeEntity : runtimeEntities)
                if (registry.all_of<T>(runtimeEntity))
                    registry.get<T>(runtimeEntity).LoadToEngine();
        };

        loadView.operator()<vke_component::Camera>();
        loadView.operator()<vke_component::RenderableObject>();
        loadView.operator()<vke_component::SkeletonAnimator>();
        loadView.operator()<vke_component::UIText>();

        for (const entt::entity runtimeEntity : runtimeEntities)
            if (registry.all_of<vke_component::RigidBody>(runtimeEntity))
                registry.get<vke_component::RigidBody>(runtimeEntity).LoadToEngine(runtimeEntity);

        for (const entt::entity runtimeEntity : runtimeEntities)
            if (registry.all_of<vke_component::Sensor>(runtimeEntity))
                registry.get<vke_component::Sensor>(runtimeEntity).LoadToEngine(runtimeEntity);

        loadView.operator()<vke_component::CharacterController>();
        loadView.operator()<vke_component::AudioSource>();
        loadView.operator()<vke_component::AudioListener>();

        for (const entt::entity runtimeEntity : runtimeEntities)
            if (registry.all_of<vke_component::DirectionalLight>(runtimeEntity))
                registry.get<vke_component::DirectionalLight>(runtimeEntity).LoadToEngine(runtimeEntity);

        for (const entt::entity runtimeEntity : runtimeEntities)
            if (registry.all_of<vke_component::PointLight>(runtimeEntity))
                registry.get<vke_component::PointLight>(runtimeEntity).LoadToEngine(runtimeEntity);

        for (const entt::entity runtimeEntity : runtimeEntities)
            if (registry.all_of<vke_component::SpotLight>(runtimeEntity))
                registry.get<vke_component::SpotLight>(runtimeEntity).LoadToEngine(runtimeEntity);

        std::vector<CSharpScriptLoadData> loadData;
        loadData.reserve(scripts.size());
        for (const PreparedScript &encoded : scripts)
        {
            loadData.push_back(CSharpScriptLoadData{
                .entity = static_cast<uint32_t>(dataToRuntime.at(encoded.entity)),
                .className = encoded.className.c_str(),
                .data = encoded.data->data(),
                .dataSize = static_cast<int32_t>(encoded.data->size()),
            });
        }

        if (!loadData.empty())
        {
            ScriptManager::Load(loadData.data(), static_cast<uint32_t>(loadData.size()));
            ScriptManager::Start(runtimeEntities);
        }
    }

    void SceneManager::unloadEntityFromEngine(entt::entity entity)
    {
        if (registry.all_of<vke_component::Camera>(entity))
            registry.get<vke_component::Camera>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::RenderableObject>(entity))
            registry.get<vke_component::RenderableObject>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::UIText>(entity))
            registry.get<vke_component::UIText>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::SkeletonAnimator>(entity))
            registry.get<vke_component::SkeletonAnimator>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::CharacterController>(entity))
            registry.get<vke_component::CharacterController>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::Sensor>(entity))
            registry.get<vke_component::Sensor>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::RigidBody>(entity))
            registry.get<vke_component::RigidBody>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::AudioSource>(entity))
            registry.get<vke_component::AudioSource>(entity).UnloadFromEngine();

        if (registry.all_of<vke_component::AudioListener>(entity))
            registry.get<vke_component::AudioListener>(entity).UnloadFromEngine();

        if (!registry.any_of<vke_component::DirectionalLight, vke_component::PointLight,
                             vke_component::SpotLight>(entity))
            return;
        auto *lightManager = vke_render::Renderer::GetInstance()->lightManager.get();

        if (lightManager->HasLight<vke_render::DirectionalLight>(entity))
            lightManager->RemoveLight<vke_render::DirectionalLight>(entity);

        if (lightManager->HasLight<vke_render::PointLight>(entity))
            lightManager->RemoveLight<vke_render::PointLight>(entity);

        if (lightManager->HasLight<vke_render::SpotLight>(entity))
            lightManager->RemoveLight<vke_render::SpotLight>(entity);
    }

    void SceneManager::dispose()
    {
        shuttingDown = true;
        pendingInstantiations.clear();
        for (const auto entity : registry.view<GameObject>())
            DestroyEntity(entity);
        // There is no next frame during shutdown. New creation is disabled above.
        while (!pendingDestroy.empty())
            ProcessDestroyRequests();
        ScriptManager::Unload();
        vke_physics::PhysicsManager::RemoveUpdateListener(physicsUpdateListenerID);
        physicsUpdateListenerID = 0;
        registry.clear();
        csharpScriptStates.clear();
        pendingDestroy.clear();
    }

    void SceneManager::physicsUpdateCallback(void *self, void *info)
    {
        SceneManager &scene = *static_cast<SceneManager *>(self);
        JPH::BodyInterface &interface = vke_physics::PhysicsManager::GetBodyInterface();
        auto view = scene.registry.view<Transform, vke_component::RigidBody>();
        for (auto &&[entity, transform, rigidbody] : view.each())
        {
            JPH::RVec3 position;
            JPH::Quat rotation;
            interface.GetPositionAndRotation(rigidbody.bodyID, position, rotation);

            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            scene.transformSystem.SetGlobalRotation(entity, glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
        }

        auto sensorView = scene.registry.view<Transform, vke_component::Sensor>();
        for (auto &&[entity, transform, sensor] : sensorView.each())
        {
            JPH::RVec3 position;
            JPH::Quat rotation;
            interface.GetPositionAndRotation(sensor.bodyID, position, rotation);

            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            scene.transformSystem.SetGlobalRotation(entity, glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
        }

        const float deltaTime = vke_physics::PhysicsManager::GetConfig().stepTime;
        auto characterView = scene.registry.view<Transform, vke_component::CharacterController>();
        for (auto &&[entity, transform, controller] : characterView.each())
        {
            controller.Update(deltaTime);
            if (controller.character == nullptr)
                continue;

            JPH::RVec3 position = controller.character->GetPosition();
            scene.transformSystem.SetGlobalPosition(entity, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
        }
    }
}
