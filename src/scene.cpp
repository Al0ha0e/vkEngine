#include <scene.hpp>

namespace vke_common
{
    SceneManager *SceneManager::instance = nullptr;

    nlohmann::json Scene::ToJSON()
    {
        nlohmann::json ret;
        ret["layers"] = layers;
        ret["maxid"] = idAllocator.id;
        nlohmann::json objectsJSON = nlohmann::json::array();

        for (auto &[id, entity] : idToEntity)
        {
            auto [obj, transform] = registry.get<GameObject, Transform>(entity);
            if (obj.layer != 1)
            {
                nlohmann::json objJSON = obj.ToJSON();
                objJSON["transform"] = transform.ToJSON();
                objJSON["parent"] = GetEntityID(transform.parent);

                nlohmann::json chidrenJSON = nlohmann::json::array();
                for (entt::entity child : transform.children)
                    chidrenJSON.push_back(GetEntityID(child));
                objJSON["children"] = std::move(chidrenJSON);

                nlohmann::json componentsJSON = nlohmann::json::array();
                componentToJSON(id, componentsJSON);
                objJSON["components"] = componentsJSON;

                objectsJSON.push_back(std::move(objJSON));
            }
        }
        ret["objects"] = std::move(objectsJSON);

        return ret;
    }

    void Scene::init(const nlohmann::json &json)
    {
        auto &lrs = json["layers"];
        for (auto &layer : lrs)
            layers.push_back(layer);

        auto &jsonObjs = json["objects"];
        for (auto &jsonObj : jsonObjs)
        {
            vke_ds::id32_t id = jsonObj["id"].get<vke_ds::id32_t>();
            entt::entity entity = registry.create();
            idToEntity[id] = entity;
            registry.emplace<GameObject>(entity, jsonObj);
            registry.emplace<Transform>(entity, jsonObj["transform"]);
        }

        transformSystem.InitializeHierarchy(jsonObjs);

        for (auto &jsonObj : jsonObjs)
        {
            vke_ds::id32_t id = jsonObj["id"].get<vke_ds::id32_t>();
            const entt::entity entity = idToEntity[id];

            auto &components = jsonObj["components"];
            for (auto &component : components)
                loadComponent(id, entity, component);
        }
    }
}