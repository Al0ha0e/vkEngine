#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
            rotation = glm::rotate(glm::mat4(1.0f), dir.x, glm::vec3(1.0f, 0.0f, 0.0f));
            rotation = glm::rotate(rotation, dir.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotation = glm::rotate(rotation, dir.z, glm::vec3(0.0f, 0.0f, 1.0f));
            translation = glm::translate(glm::mat4(1.0f), pos);
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

        virtual void OnTransformed(TransformParameter &param) {}
    };

    class GameObject
    {
    public:
        TransformParameter transform;
        std::vector<std::unique_ptr<Component>> components;

        GameObject(TransformParameter &tp) : transform(tp){};

        ~GameObject() {}

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
    };
}

#endif