#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <iostream>
#include <memory>

namespace vke_common
{

    struct TransformParameter
    {
        glm::mat4 model;
        glm::vec3 position;
        glm::vec3 localPosition;
        glm::vec3 lossyScale;
        glm::vec3 localScale;
        glm::quat rotation;
        glm::quat localRotation;

        TransformParameter()
            : model(1), position(0), localPosition(0), lossyScale(1), localScale(1), rotation(glm::vec3(0)), localRotation(glm::vec3(0))
        {
            init();
        }

        TransformParameter(glm::vec3 pos, glm::vec3 scl, glm::quat rot)
            : localPosition(pos), localScale(scl), localRotation(rot)
        {
            init();
        }

        TransformParameter(const TransformParameter &fa, glm::vec3 pos, glm::vec3 scl, glm::quat rot)
            : localPosition(pos), localScale(scl), localRotation(rot)
        {
            initWithParent(fa);
        }

        TransformParameter(nlohmann::json &json)
        {
            auto pos = json["pos"];
            auto scl = json["scl"];
            auto rot = json["rot"];
            localPosition = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
            localScale = glm::vec3(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());
            localRotation = glm::quat(rot[3].get<float>(), rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>());
            init();
        }

        TransformParameter(const TransformParameter &fa, nlohmann::json &json)
        {
            auto pos = json["pos"];
            auto scl = json["scl"];
            auto rot = json["rot"];
            localPosition = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
            localScale = glm::vec3(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());
            localRotation = glm::quat(rot[3].get<float>(), rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>());
            initWithParent(fa);
        }

        void RemoveParent()
        {
            localPosition = position;
            localScale = lossyScale;
            localRotation = rotation;
            calcModelMatrix();
        }

        void SetParent(const TransformParameter &fa)
        {
            glm::mat4 inv(glm::inverse(fa.model));
            localPosition = inv * glm::vec4(position, 1);
            localRotation = glm::inverse(fa.rotation) * rotation;
            localScale = glm::mat4_cast(glm::inverse(localRotation)) * inv * glm::mat4_cast(rotation) * glm::vec4(lossyScale, 0);
            calcModelMatrixWithParent(fa.model);
        }

        void SetLocalPosition(glm::vec3 &pos)
        {
            localPosition = pos;
            position = pos;
            calcModelMatrix();
        }

        void SetLocalPositionWithParent(const glm::mat4 &fa, glm::vec3 &pos)
        {
            localPosition = pos;
            calcModelMatrixWithParent(fa);
            position = model[3];
        }

        void TranslateLocal(glm::vec3 &det)
        {
            glm::vec3 localDet = glm::mat4_cast(localRotation) * glm::vec4(det, 0);
            localPosition += localDet;
            position += localDet;
            calcModelMatrix();
        }

        void TranslateLocalWithParent(const glm::mat4 &fa, glm::vec3 &det)
        {
            glm::vec3 localDet = glm::mat4_cast(localRotation) * glm::vec4(det, 0);
            localPosition += localDet;
            calcModelMatrixWithParent(fa);
            position = model[3];
        }

        void TranslateGlobal(glm::vec3 &det)
        {
            position += det;
            localPosition += det;
            calcModelMatrix();
        }

        void TranslateGlobalWithParent(const glm::mat4 &fa, glm::vec3 &det)
        {
            position += det;
            localPosition = glm::inverse(fa) * glm::vec4(position, 1);
            calcModelMatrixWithParent(fa);
        }

        void SetLocalRotation(glm::quat &rot)
        {
            localRotation = rot;
            rotation = rot;
            calcModelMatrix();
        }

        void SetLocalRotationWithParent(const TransformParameter &fa, glm::quat &rot)
        {
            localRotation = rot;
            rotation = fa.rotation * localRotation;
            calcModelMatrixWithParent(fa.model);
        }

        void RotateLocal(float det, glm::vec3 &axis)
        {
            localRotation = glm::rotate(localRotation, det, axis);
            rotation = localRotation;
            calcModelMatrix();
        }

        void RotateLocalWithParent(const TransformParameter &fa, float det, glm::vec3 &axis)
        {
            localRotation = glm::rotate(localRotation, det, axis);
            rotation = fa.rotation * localRotation;
            calcModelMatrixWithParent(fa.model);
        }

        void RotateGlobal(float det, glm::vec3 &axis)
        {
            glm::vec3 gaxis = glm::mat4_cast(glm::inverse(rotation)) * glm::vec4(axis, 0);
            localRotation = glm::rotate(localRotation, det, gaxis);
            rotation = localRotation;
            calcModelMatrix();
        }

        void RotateGlobalWithParent(const TransformParameter &fa, float det, glm::vec3 &axis)
        {
            glm::vec3 gaxis = glm::mat4_cast(glm::inverse(rotation)) * glm::vec4(axis, 0);
            localRotation = glm::rotate(localRotation, det, gaxis);
            rotation = fa.rotation * localRotation;
            calcModelMatrixWithParent(fa.model);
        }

        void SetLocalScale(glm::vec3 &scale)
        {
            localScale = scale;
            lossyScale = scale;
            calcModelMatrix();
        }

        void SetLocalScaleWithParent(const TransformParameter &fa, glm::vec3 &scale)
        {
            localScale = scale;
            lossyScale = glm::mat4_cast(glm::inverse(rotation)) * fa.model * glm::mat4_cast(localRotation) * glm::vec4(localScale, 0);
            calcModelMatrixWithParent(fa.model);
        }

        void Scale(glm::vec3 &scale)
        {
            localScale += scale;
            lossyScale = localScale;
            calcModelMatrix();
        }

        void ScaleWithParent(const TransformParameter &fa, glm::vec3 &scale)
        {
            localScale += scale;
            lossyScale = glm::mat4_cast(glm::inverse(rotation)) * fa.model * glm::mat4_cast(localRotation) * glm::vec4(localScale, 0);
            calcModelMatrixWithParent(fa.model);
        }

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"pos\": [";
            for (int i = 0; i < 3; i++)
                ret += std::to_string(localPosition[i]) + ",";
            ret[ret.length() - 1] = ']';

            ret += ",\n\"scl\": [";
            for (int i = 0; i < 3; i++)
                ret += std::to_string(localScale[i]) + ",";
            ret[ret.length() - 1] = ']';

            ret += ",\n\"rot\": [";
            for (int i = 0; i < 4; i++)
                ret += std::to_string(localRotation[i]) + ",";
            ret[ret.length() - 1] = ']';
            ret += "\n}";
            return ret;
        }

        void UpdateWithParent(const TransformParameter &fa)
        {
            initWithParent(fa);
        }

    private:
        void calcModelMatrix()
        {
            model = glm::translate(glm::mat4(1.0f), localPosition) *
                    glm::mat4_cast(localRotation) *
                    glm::scale(glm::mat4(1.0f), localScale);
        }

        void calcModelMatrixWithParent(const glm::mat4 &fa)
        {
            model = fa * glm::translate(glm::mat4(1.0f), localPosition) *
                    glm::mat4_cast(localRotation) *
                    glm::scale(glm::mat4(1.0f), localScale);
        }

        void init()
        {
            calcModelMatrix();
            position = localPosition;
            lossyScale = localScale;
            rotation = localRotation;
        }

        void initWithParent(const TransformParameter &fa)
        {
            calcModelMatrixWithParent(fa.model);
            position = model[3];
            rotation = fa.rotation * localRotation;
            lossyScale = glm::mat4_cast(glm::inverse(rotation)) * fa.model * glm::mat4_cast(localRotation) * glm::vec4(localScale, 0);
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
        GameObject *parent;
        std::map<int, GameObject *> children;

        TransformParameter transform;
        std::vector<std::unique_ptr<Component>> components;

        GameObject(TransformParameter &tp) : parent(nullptr), transform(tp) {};

        GameObject(nlohmann::json &json, std::map<int, std::unique_ptr<GameObject>> &objects)
            : parent(nullptr), id(json["id"]), transform(json["transform"])
        {
            auto &comps = json["components"];
            for (auto &comp : comps)
                components.push_back(loadComponent(comp));

            auto &chs = json["children"];
            for (auto &ch : chs)
            {
                GameObject *child = new GameObject(this, ch, objects);
                children[ch["id"]] = child;
                objects[child->id] = std::unique_ptr<GameObject>(child);
            }
        }

        GameObject(GameObject *fa, nlohmann::json &json, std::map<int, std::unique_ptr<GameObject>> &objects)
            : parent(fa), id(json["id"]), transform(fa->transform, json["transform"])
        {
            auto &comps = json["components"];
            for (auto &comp : comps)
                components.push_back(loadComponent(comp));

            auto &chs = json["children"];
            for (auto &ch : chs)
            {
                GameObject *child = new GameObject(this, ch, objects);
                children[child->id] = child;
                objects[child->id] = std::unique_ptr<GameObject>(child);
            }
        }

        ~GameObject() {}

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"id\": " + std::to_string(id) + ",\n";
            ret += "\"parent\": " + std::to_string(parent ? parent->id : 0) + ",\n";

            ret += "\"transform\": " + transform.ToJSON();

            ret += ",\n\"components\": [";
            for (auto &component : components)
                ret += "\n" + component->ToJSON() + ",";
            ret[ret.length() - 1] = ']';

            ret += ",\n\"children\": [ ";
            for (auto &kv : children)
                ret += "\n" + kv.second->ToJSON() + ",";
            ret[ret.length() - 1] = ']';
            ret += "\n}";

            return ret;
        }

        void AddComponent(std::unique_ptr<Component> &&component)
        {
            components.push_back(std::forward<std::unique_ptr<Component>>(component));
        }

        void SetParent(GameObject *fa)
        {
            if (fa == parent)
                return;
            if (parent != nullptr)
            {
                parent->children.erase(id);
                transform.RemoveParent();
            }
            parent = fa;
            if (fa == nullptr)
            {
                updateTransform(true);
                return;
            }

            transform.SetParent(fa->transform);
            fa->children[id] = this;
            updateTransform(true);
        }

        void SetLocalPosition(glm::vec3 &position)
        {
            parent ? transform.SetLocalPositionWithParent(parent->transform.model, position) : transform.SetLocalPosition(position);
            updateTransform(true);
        }

        void SetLocalRotation(glm::quat &rotation)
        {
            parent ? transform.SetLocalRotationWithParent(parent->transform, rotation) : transform.SetLocalRotation(rotation);
            updateTransform(true);
        }

        void SetLocalScale(glm::vec3 &scale)
        {
            parent ? transform.SetLocalScaleWithParent(parent->transform, scale) : transform.SetLocalScale(scale);
            updateTransform(true);
        }

        void RotateGlobal(float det, glm::vec3 &axis)
        {
            parent ? transform.RotateGlobalWithParent(parent->transform, det, axis) : transform.RotateGlobal(det, axis);
            updateTransform(true);
        }

        void RotateLocal(float det, glm::vec3 &axis)
        {
            parent ? transform.RotateLocalWithParent(parent->transform, det, axis) : transform.RotateLocal(det, axis);
            updateTransform(true);
        }

        void TranslateLocal(glm::vec3 det)
        {
            parent ? transform.TranslateLocalWithParent(parent->transform.model, det) : transform.TranslateLocal(det);
            updateTransform(true);
        }

        void TranslateGlobal(glm::vec3 det)
        {
            parent ? transform.TranslateGlobalWithParent(parent->transform.model, det) : transform.TranslateGlobal(det);
            updateTransform(true);
        }

        void Scale(glm::vec3 scale)
        {
            parent ? transform.ScaleWithParent(parent->transform, scale) : transform.Scale(scale);
            updateTransform(true);
        }

    private:
        void updateTransform(bool first)
        {
            if (!first)
                transform.UpdateWithParent(parent->transform);

            for (auto &component : components)
                component->OnTransformed(transform);

            for (auto &kv : children)
                kv.second->updateTransform(false);
        }

        std::unique_ptr<Component> loadComponent(nlohmann::json &json);
    };
}

#endif