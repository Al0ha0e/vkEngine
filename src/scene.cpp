#include <scene.hpp>

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
        : layers(json.value("layers", std::vector<std::string>{"default", "editor"})),
          maxID(json.value("maxid", 1u))
    {
        const nlohmann::json &objects = json["objects"];

        for (const nlohmann::json &object : objects)
        {
            const vke_ds::id32_t id = object["id"];
            entt::entity entity = registry.create();
            idToEntity[id] = entity;

            std::string name = object["name"];
            registry.emplace<GameObject>(
                entity, id, name, object["layer"].get<int>(), object["static"].get<bool>());
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
            {"layers", layers},
            {"maxid", maxID},
            {"objects", nlohmann::json::array()}};

        for (const auto &[id, entity] : idToEntity)
        {
            const GameObject &object = registry.get<GameObject>(entity);
            const TransformData &transform = registry.get<TransformData>(entity);
            nlohmann::json objectJSON = {
                {"id", object.id},
                {"static", object.isStatic},
                {"name", object.name},
                {"layer", object.layer},
                {"transform", transform.ToJSON()},
                {"parent", 0},
                {"components", nlohmann::json::array()}};

            auto parentIt = parents.find(entity);
            if (parentIt != parents.end() && parentIt->second != entt::null)
                objectJSON["parent"] = registry.get<GameObject>(parentIt->second).id;

            componentToJSON(entity, objectJSON["components"]);
            result["objects"].push_back(std::move(objectJSON));
        }

        return result;
    }

    void Scene::FillData(SceneData &data) const
    {
        data.layers = layers;
        data.maxID = idAllocator.id;
        data.registry.clear();
        data.idToEntity.clear();
        data.parents.clear();

        std::unordered_map<entt::entity, entt::entity> runtimeToData;

        for (const auto &[id, runtimeEntity] : idToEntity)
        {
            const GameObject &object = registry.get<GameObject>(runtimeEntity);
            if (object.layer == 1)
                continue;

            entt::entity dataEntity = data.registry.create();
            data.idToEntity[id] = dataEntity;
            runtimeToData[runtimeEntity] = dataEntity;

            std::string name = object.name;
            data.registry.emplace<GameObject>(
                dataEntity, object.id, name, object.layer, object.isStatic);
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
    }

    void Scene::init(const SceneData &data)
    {
        layers = data.layers;
        std::unordered_map<entt::entity, entt::entity> dataToRuntime;

        for (const auto &[id, dataEntity] : data.idToEntity)
        {
            const GameObject &object = data.registry.get<GameObject>(dataEntity);
            entt::entity runtimeEntity = registry.create();
            idToEntity[id] = runtimeEntity;
            dataToRuntime[dataEntity] = runtimeEntity;

            std::string name = object.name;
            registry.emplace<GameObject>(
                runtimeEntity, object.id, name, object.layer, object.isStatic);
            registry.emplace<Transform>(
                runtimeEntity, data.registry.get<TransformData>(dataEntity));
        }

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
        for (const auto &[id, entity] : idToEntity)
            if (registry.get<Transform>(entity).parent == entt::null)
                updateHierarchy(entity);

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
    }

}
