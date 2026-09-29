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

    SceneData::SceneData(const nlohmann::json &json)
        : maxID(json.value("maxid", 1u))
    {
        const nlohmann::json &objects = json["objects"];

        for (const nlohmann::json &object : objects)
        {
            const vke_ds::id32_t id = object["id"];
            entt::entity entity = registry.create();
            idToEntity[id] = entity;

            std::string name = object["name"];
            registry.emplace<GameObject>(
                entity, name, object["static"].get<bool>());
            registry.emplace<TransformData>(entity, object["transform"]);
        }

        for (const nlohmann::json &object : objects)
        {
            const entt::entity entity = idToEntity.at(object["id"].get<vke_ds::id32_t>());
            const vke_ds::id32_t parentID = object.value("parent", 0u);
            parents[entity] = parentID == 0 ? entt::null : idToEntity.at(parentID);

            for (const nlohmann::json &component : object["components"])
                loadComponent(entity, component);
        }
    }

    nlohmann::json SceneData::ToJSON() const
    {
        nlohmann::json result = {
            {"maxid", maxID},
            {"objects", nlohmann::json::array()}};

        std::unordered_map<entt::entity, vke_ds::id32_t> entityToID;
        entityToID.reserve(idToEntity.size());
        for (const auto &[id, entity] : idToEntity)
            entityToID.emplace(entity, id);

        for (const auto &[id, entity] : idToEntity)
        {
            const GameObject &object = registry.get<GameObject>(entity);
            const TransformData &transform = registry.get<TransformData>(entity);
            nlohmann::json objectJSON = {
                {"id", id},
                {"static", object.isStatic},
                {"name", object.name},
                {"transform", transform.ToJSON()},
                {"parent", 0},
                {"components", nlohmann::json::array()}};

            auto parentIt = parents.find(entity);
            if (parentIt != parents.end() && parentIt->second != entt::null)
                objectJSON["parent"] = entityToID.at(parentIt->second);

            componentToJSON(entity, objectJSON["components"]);
            result["objects"].push_back(std::move(objectJSON));
        }

        return result;
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
        return exportEntities(entities);
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
        return data;
    }

    SceneManager::EntityMap SceneManager::instantiateSceneData(const SceneData &data)
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
            registry.emplace<Transform>(
                runtimeEntity, data.registry.get<TransformData>(dataEntity));
        }

        ///////////////////////////// construct parent relation & update transform /////////////////////////

        for (const auto &[dataEntity, runtimeEntity] : dataToRuntime)
        {
            Transform &transform = registry.get<Transform>(runtimeEntity);
            auto parentIt = data.parents.find(dataEntity);
            if (parentIt == data.parents.end() || parentIt->second == entt::null)
                continue;

            transform.parent = dataToRuntime.at(parentIt->second);
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
            if (registry.get<Transform>(runtimeEntity).parent == entt::null)
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

    void SceneManager::unloadSceneData(const EntityMap &dataToRuntime)
    {
        for (const auto &[dataEntity, runtimeEntity] : dataToRuntime)
            DestroyEntity(runtimeEntity);
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

    void SceneManager::loadEntitiesToEngine(const EntityMap &dataToRuntime)
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

        struct EncodedScript
        {
            uint32_t entity;
            std::string className;
            TypeInfoDataPtr data;
        };
        std::vector<EncodedScript> encodedScripts;
        for (const entt::entity runtimeEntity : runtimeEntities)
        {
            auto scriptIt = csharpScriptStates.find(runtimeEntity);
            if (scriptIt == csharpScriptStates.end())
                continue;
            for (const auto &[className, state] : scriptIt->second)
            {
                TypeInfoPtr type = ScriptManager::GetInstance()->FindTypeInfo(className);
                VKE_FATAL_IF(!type, "No TypeInfo exists for EntityScript '{}'", className)
                auto encoded = type->EncodeBinaryFromJson(state.serializedData);
                if (!encoded)
                {
                    VKE_FATAL("Failed to encode EntityScript '{}' state at {}: {}",
                              className, encoded.error().path,
                              ToString(encoded.error().code))
                }
                encodedScripts.push_back(
                    EncodedScript{
                        static_cast<uint32_t>(runtimeEntity),
                        className,
                        std::move(*encoded)});
            }
        }

        std::vector<CSharpScriptLoadData> loadData;
        loadData.reserve(encodedScripts.size());
        for (const EncodedScript &encoded : encodedScripts)
        {
            loadData.push_back(CSharpScriptLoadData{
                .entity = encoded.entity,
                .className = encoded.className.c_str(),
                .data = encoded.data->data(),
                .dataSize = static_cast<int32_t>(encoded.data->size()),
            });
        }

        ScriptManager::Load(loadData.data(), static_cast<uint32_t>(loadData.size()));
        ScriptManager::Start(runtimeEntities);
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
