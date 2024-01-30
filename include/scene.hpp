#ifndef SCENE_H
#define SCENE_H

#include <gameobject.hpp>
#include <memory>
#include <iostream>

namespace vke_common
{
    class Scene
    {
    public:
        std::vector<std::unique_ptr<GameObject>> objects;

        void AddObject(std::unique_ptr<GameObject> &&object)
        {
            objects.push_back(std::forward<std::unique_ptr<GameObject>>(object));
        }

        ~Scene() {}
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
        }
    };
}

#endif
