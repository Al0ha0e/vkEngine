#ifndef SCENE_H
#define SCENE_H

#include <ds/id_allocator.hpp>
#include <asset.hpp>
#include <gameobject.hpp>

#include <memory>
#include <iostream>
#include <fstream>
#include <map>

namespace vke_common
{
    class Scene
    {
    public:
        vke_common::AssetHandle handle;
        std::string path;
        std::vector<std::string> layers;
        std::map<vke_ds::id64_t, std::unique_ptr<GameObject>> objects;

        Scene() : idAllocator(1), layers({"default", "editor"}) {}

        Scene(nlohmann::json &json) : idAllocator(json["maxid"])
        {
            init(json);
        }

        Scene(const std::string &pth, nlohmann::json &json) : path(pth), idAllocator(json["maxid"])
        {
            init(json);
        }

        ~Scene() {}

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"maxid\": " + std::to_string(idAllocator.id) + ",\n";

            ret += "\"layers\": [";
            for (auto &layer : layers)
                ret += "\"" + layer + "\",";
            ret[ret.length() - 1] = ']';
            ret += ",\n";

            ret += "\"objects\": [ ";
            for (auto &obj : objects)
                if (obj.second->parent == nullptr && obj.second->layer != 1)
                    ret += "\n" + obj.second->ToJSON() + ",";
            ret[ret.length() - 1] = ']';
            ret += "\n}";

            return ret;
        }

        void AddObject(std::unique_ptr<GameObject> &&object)
        {
            vke_ds::id64_t id = idAllocator.Alloc();
            object->id = id;
            objects[id] = std::forward<std::unique_ptr<GameObject>>(object);
        }

        void RemoveObject(vke_ds::id64_t id)
        {
            auto &object = objects[id];
            if (object->parent != nullptr)
                object->parent->RemoveChild(id);

            std::vector<vke_ds::id64_t> objsToBeRemoved = {id};
            int now = 0;
            while (now < objsToBeRemoved.size())
            {
                vke_ds::id64_t oid = objsToBeRemoved[now++];
                auto &object = objects[oid];
                for (auto &kv : object->children)
                    objsToBeRemoved.push_back(kv.first);
            }

            for (vke_ds::id64_t oid : objsToBeRemoved)
                objects.erase(oid);
        }

        GameObject *GetObject(vke_ds::id64_t id)
        {
            auto it = objects.find(id);
            if (it == objects.end())
                return nullptr;
            return it->second.get();
        }

    private:
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> idAllocator;

        void init(nlohmann::json &json);
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
            if (instance == nullptr)
                throw std::runtime_error("SceneManager not initialized!");
            return instance;
        }

        static SceneManager *Init()
        {
            instance = new SceneManager();
            return instance;
        }

        static void Dispose()
        {
            delete instance;
        }

        static void SetCurrentScene(std::unique_ptr<Scene> &&scene)
        {
            std::exchange<std::unique_ptr<Scene>>(instance->currentScene, std::forward<std::unique_ptr<Scene>>(scene));
        }

        static void LoadScene(const std::string &pth)
        {
            nlohmann::json json(vke_common::AssetManager::LoadJSON(pth));
            SetCurrentScene(std::make_unique<Scene>(pth, json));
        }

        static void SaveScene(const std::string &pth)
        {
            if (instance->currentScene != nullptr)
            {
                std::ofstream ofs(pth);
                ofs << instance->currentScene->ToJSON();
                ofs.close();
            }
        }
    };
}

#endif
