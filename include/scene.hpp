#ifndef SCENE_H
#define SCENE_H

#include <ds/id_allocator.hpp>
#include <resource.hpp>
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
        std::map<int, std::unique_ptr<GameObject>> objects;

        Scene() : idAllocator(1) {}

        Scene(nlohmann::json &json) : idAllocator(json["maxid"])
        {
            auto &objs = json["objects"];
            for (auto &obj : objs)
            {
                std::unique_ptr<GameObject> object = std::make_unique<GameObject>(obj, objects);
                objects[object->id] = std::move(object);
            }
        }

        ~Scene() {}

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"maxid\": " + std::to_string(idAllocator.id) + ",\n";
            ret += "\"objects\": [";
            for (auto &obj : objects)
                if (obj.second->parent == nullptr)
                    ret += "\n" + obj.second->ToJSON() + ",";
            ret[ret.length() - 1] = ']';
            ret += "\n}";

            return ret;
        }

        void AddObject(std::unique_ptr<GameObject> &&object)
        {
            int id = idAllocator.Alloc();
            object->id = id;
            objects[id] = std::forward<std::unique_ptr<GameObject>>(object);
        }

        GameObject *GetObject(int id)
        {
            auto it = objects.find(id);
            if (it == objects.end())
                return nullptr;
            return it->second.get();
        }

    private:
        vke_ds::NaiveIDAllocator<int> idAllocator;
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
            nlohmann::json json(vke_common::ResourceManager::LoadJSON(pth));
            SetCurrentScene(std::make_unique<Scene>(json));
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
