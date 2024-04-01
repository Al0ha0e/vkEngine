#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <iostream>
#include <memory>

namespace vke_common
{

    struct TransformParameter
    {
        glm::mat4 model;
        glm::mat4 translation;
        glm::mat4 rotation;
        glm::vec3 position;
        glm::vec3 direction;

        TransformParameter() {}
        TransformParameter(glm::vec3 pos, glm::vec3 dir) : position(pos), direction(dir)
        {
            init();
        }

        TransformParameter(nlohmann::json &json)
        {
            auto pos = json["pos"];
            auto dir = json["dir"];
            position = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
            direction = glm::vec3(dir[0].get<float>(), dir[1].get<float>(), dir[2].get<float>());
            init();
        }

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"pos\": [";
            for (int i = 0; i < 3; i++)
                ret += std::to_string(position[i]) + ",";
            ret[ret.length() - 1] = ']';
            ret += ",\n\"dir\": [";
            for (int i = 0; i < 3; i++)
                ret += std::to_string(direction[i]) + ",";
            ret[ret.length() - 1] = ']';
            ret += "\n}";
            return ret;
        }

    private:
        void init()
        {
            rotation = glm::rotate(glm::mat4(1.0f), direction.x, glm::vec3(1.0f, 0.0f, 0.0f));
            rotation = glm::rotate(rotation, direction.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotation = glm::rotate(rotation, direction.z, glm::vec3(0.0f, 0.0f, 1.0f));
            translation = glm::translate(glm::mat4(1.0f), position);
            model = translation * rotation;
        }
    };

    class GameObject;

    class Component
    {
    public:
        GameObject *gameObject;
        Component(GameObject *obj) : gameObject(obj) {}
        virtual ~Component() {}

        virtual std::string ToJSON() = 0;

        virtual void OnTransformed(TransformParameter &param) {}
    };

    class GameObject
    {
    public:
        int id;
        TransformParameter transform;
        std::vector<std::unique_ptr<Component>> components;

        GameObject(TransformParameter &tp) : transform(tp){};

        GameObject(nlohmann::json &json) : id(json["id"]), transform(json["transform"])
        {
            auto &comps = json["components"];
            for (auto &comp : comps)
                components.push_back(loadComponent(comp));
        }

        ~GameObject() {}

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"id\": " + std::to_string(id) + ",\n";
            ret += "\"transform\": " + transform.ToJSON();
            ret += ",\n\"components\": [";
            for (auto &component : components)
                ret += "\n" + component->ToJSON() + ",";
            ret[ret.length() - 1] = ']';
            ret += "\n}";
            return ret;
        }

        void AddComponent(std::unique_ptr<Component> &&component)
        {
            components.push_back(std::forward<std::unique_ptr<Component>>(component));
        }

        void RotateGlobal(float det, glm::vec3 &axis)
        {
            glm::vec3 gaxis = glm::transpose(transform.rotation) * glm::vec4(axis, 0.0f);
            transform.rotation = glm::rotate(transform.rotation, det, glm::normalize(gaxis));
            updateTransform();
        }

        void RotateLocal(float det, glm::vec3 &axis)
        {
            transform.rotation = glm::rotate(transform.rotation, det, glm::normalize(axis));
            updateTransform();
        }

        void TranslateLocal(glm::vec3 det)
        {
            glm::vec3 gdet = transform.rotation * glm::vec4(det, 0.0f);
            transform.translation = glm::translate(transform.translation, gdet);
            transform.position = transform.translation * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            updateTransform();
        }

        void TranslateGlobal(glm::vec3 det)
        {
            transform.translation = glm::translate(transform.translation, det);
            transform.position = transform.translation * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            updateTransform();
        }

    private:
        void updateTransform()
        {
            transform.model = transform.translation * transform.rotation;
            for (auto &component : components)
            {
                component->OnTransformed(transform);
            }
        }

        std::unique_ptr<Component> loadComponent(nlohmann::json &json);
    };
}

#endif