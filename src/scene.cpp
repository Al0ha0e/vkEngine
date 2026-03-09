#include <scene.hpp>
#include <unordered_set>

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
                componentToJSON(entity, componentsJSON);
                objJSON["components"] = componentsJSON;

                objectsJSON.push_back(std::move(objJSON));
            }
        }
        ret["objects"] = std::move(objectsJSON);

        return ret;
    }

    void Scene::dfs(entt::entity entity, Transform &transform, std::unordered_set<entt::entity> &visited)
    {
        visited.insert(entity);

        if (transform.parent != entt::null)
            transform.SetParentFixedLocal(registry.get<Transform>(transform.parent));

        for (auto child : transform.children)
        {
            VKE_FATAL_IF(visited.find(child) != visited.end(), "SCENE OBJ CONTAIN LOOP")
            Transform &childTransform = registry.get<Transform>(child);
            dfs(child, childTransform, visited);
        }
    }

    void Scene::init(const nlohmann::json &json)
    {
        physicsUpdateListenerID = vke_physics::PhysicsManager::RegisterUpdateListener(this,
                                                                                      std::function<void(void *, void *)>(physicsUpdateCallback));

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

        for (auto &jsonObj : jsonObjs)
        {
            vke_ds::id32_t id = jsonObj["id"].get<vke_ds::id32_t>();
            Transform &transform = registry.get<Transform>(idToEntity[id]);

            vke_ds::id32_t parentId = jsonObj["parent"].get<vke_ds::id32_t>();
            transform.parent = parentId ? idToEntity.at(parentId) : entt::null;

            for (auto &jsonChild : jsonObj["children"])
                transform.children.insert(idToEntity[jsonChild.get<vke_ds::id32_t>()]);
        }

        std::unordered_set<entt::entity> visited;

        for (auto &[id, entity] : idToEntity)
        {
            Transform &transform = registry.get<Transform>(entity);
            if (transform.parent == entt::null)
                dfs(entity, transform, visited);
        }

        for (auto &jsonObj : jsonObjs)
        {
            vke_ds::id32_t id = jsonObj["id"].get<vke_ds::id32_t>();
            auto &components = jsonObj["components"];
            for (auto &component : components)
                loadComponent(idToEntity[id], component);
        }
    }
}