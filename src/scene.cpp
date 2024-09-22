#include <scene.hpp>

namespace vke_common
{
    SceneManager *SceneManager::instance = nullptr;

    void Scene::init(nlohmann::json &json)
    {
        auto &lrs = json["layers"];
        for (auto &layer : lrs)
            layers.push_back(layer);

        auto &objs = json["objects"];
        for (auto &obj : objs)
        {
            std::unique_ptr<GameObject> object = std::make_unique<GameObject>(obj, objects);
            objects[object->id] = std::move(object);
        }
    }
}