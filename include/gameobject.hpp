#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <iostream>
#include <memory>
#include <ds/id_allocator.hpp>

namespace vke_common
{

    struct TransformParameter
    {
        glm::mat4 model;
        glm::vec3 localPosition;
        glm::vec3 localScale;
        glm::quat localRotation;

        TransformParameter()
            : model(1), localPosition(0), localScale(1), localRotation(glm::vec3(0))
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

        TransformParameter(const nlohmann::json &json)
        {
            auto pos = json["pos"];
            auto scl = json["scl"];
            auto rot = json["rot"];
            localPosition = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
            localScale = glm::vec3(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());
            localRotation = glm::normalize(glm::quat(rot[3].get<float>(), rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>()));
            init();
        }

        TransformParameter(const TransformParameter &fa, const nlohmann::json &json)
        {
            auto pos = json["pos"];
            auto scl = json["scl"];
            auto rot = json["rot"];
            localPosition = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
            localScale = glm::vec3(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());
            localRotation = glm::normalize(glm::quat(rot[3].get<float>(), rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>()));
            initWithParent(fa);
        }

        glm::vec3 GetGlobalPosition() const
        {
            return glm::vec3(model[3]);
        }

        glm::vec3 GetLossyScale() const
        {
            return glm::vec3(
                glm::length(glm::vec3(model[0])),
                glm::length(glm::vec3(model[1])),
                glm::length(glm::vec3(model[2])));
        }

        glm::quat GetGlobalRotation() const
        {
            glm::vec3 scale = GetLossyScale();
            glm::mat3 rotMat(
                glm::vec3(model[0]) / scale.x,
                glm::vec3(model[1]) / scale.y,
                glm::vec3(model[2]) / scale.z);
            return glm::quat_cast(rotMat);
        }

        void RemoveParent()
        {
            localPosition = GetGlobalPosition();
            localScale = GetLossyScale();
            localRotation = glm::normalize(GetGlobalRotation());
            calcModelMatrix();
        }

        void SetParent(const TransformParameter &fa)
        {
            glm::mat4 inv(glm::inverse(fa.model));

            localPosition = inv * glm::vec4(GetGlobalPosition(), 1);
            localRotation = glm::normalize(glm::inverse(fa.GetGlobalRotation()) * GetGlobalRotation());
            localScale = GetLossyScale() / fa.GetLossyScale();
            calcModelMatrixWithParent(fa.model);
        }

        void SetLocalPosition(const glm::vec3 &pos)
        {
            localPosition = pos;
            calcModelMatrix();
        }

        void SetLocalPositionWithParent(const glm::mat4 &fa, const glm::vec3 &pos)
        {
            localPosition = pos;
            calcModelMatrixWithParent(fa);
        }

        void TranslateLocal(const glm::vec3 &det)
        {
            localPosition += localRotation * det;
            calcModelMatrix();
        }

        void TranslateLocalWithParent(const glm::mat4 &fa, const glm::vec3 &det)
        {
            localPosition += localRotation * det;
            calcModelMatrixWithParent(fa);
        }

        void TranslateGlobal(const glm::vec3 &det)
        {
            localPosition += det;
            calcModelMatrix();
        }

        void TranslateGlobalWithParent(const glm::mat4 &fa, const glm::vec3 &det)
        {
            localPosition = glm::inverse(fa) * glm::vec4(GetGlobalPosition() + det, 1.0f);
            calcModelMatrixWithParent(fa);
        }

        void SetLocalRotation(const glm::quat &rot)
        {
            localRotation = glm::normalize(rot);
            calcModelMatrix();
        }

        void SetLocalRotationWithParent(const TransformParameter &fa, const glm::quat &rot)
        {
            localRotation = glm::normalize(rot);
            calcModelMatrixWithParent(fa.model);
        }

        void RotateLocal(const float det, const glm::vec3 &axis)
        {
            localRotation = glm::normalize(glm::rotate(localRotation, det, axis));
            calcModelMatrix();
        }

        void RotateLocalWithParent(const TransformParameter &fa, const float det, const glm::vec3 &axis)
        {
            localRotation = glm::normalize(glm::rotate(localRotation, det, axis));
            calcModelMatrixWithParent(fa.model);
        }

        void RotateGlobal(const float det, const glm::vec3 &axis)
        {
            localRotation = glm::normalize(glm::angleAxis(det, glm::normalize(axis)) * localRotation);
            calcModelMatrix();
        }

        void RotateGlobalWithParent(const TransformParameter &fa, const float det, const glm::vec3 &axis)
        {
            glm::quat parentRot = fa.GetGlobalRotation();
            localRotation = glm::normalize(glm::inverse(parentRot) * glm::angleAxis(det, glm::normalize(axis)) * parentRot * localRotation);
            calcModelMatrixWithParent(fa.model);
        }

        void SetLocalScale(const glm::vec3 &scale)
        {
            localScale = scale;
            calcModelMatrix();
        }

        void SetLocalScaleWithParent(const TransformParameter &fa, const glm::vec3 &scale)
        {
            localScale = scale;
            calcModelMatrixWithParent(fa.model);
        }

        void Scale(const glm::vec3 &scale)
        {
            localScale *= scale;
            calcModelMatrix();
        }

        void ScaleWithParent(const TransformParameter &fa, const glm::vec3 &scale)
        {
            localScale *= scale;
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
        }

        void initWithParent(const TransformParameter &fa)
        {
            calcModelMatrixWithParent(fa.model);
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
        vke_ds::id64_t id;
        int layer;
        bool isStatic;
        char name[32];
        GameObject *parent;
        TransformParameter transform;
        std::map<vke_ds::id64_t, GameObject *> children;
        std::vector<std::unique_ptr<Component>> components;

        GameObject(TransformParameter &tp) : parent(nullptr), layer(0), transform(tp), isStatic(false)
        {
            memcpy(name, "new object", 13);
        };

        GameObject(const nlohmann::json &json, std::map<vke_ds::id64_t, std::unique_ptr<GameObject>> &objects)
            : parent(nullptr), id(json["id"]), isStatic(json["static"]), layer(json["layer"]), transform(json["transform"])
        {
            init(json, objects);
        }

        GameObject(GameObject *fa, const nlohmann::json &json, std::map<vke_ds::id64_t, std::unique_ptr<GameObject>> &objects)
            : parent(fa), id(json["id"]), isStatic(json["static"]), layer(json["layer"]), transform(fa->transform, json["transform"])
        {
            init(json, objects);
        }

        ~GameObject() {}

        std::string ToJSON()
        {
            std::string ret = "{\n";
            ret += "\"id\": " + std::to_string(id) + ",\n";
            ret += isStatic ? "\"static\": true,\n" : "\"static\": false,\n";
            ret += "\"name\": \"" + std::string(name) + "\",\n";
            ret += "\"layer\": " + std::to_string(layer) + ",\n";
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
                parent->RemoveChild(id);
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

        void RemoveChild(vke_ds::id64_t id)
        {
            children.erase(id);
        }

        void SetLocalPosition(const glm::vec3 &position)
        {
            parent ? transform.SetLocalPositionWithParent(parent->transform.model, position) : transform.SetLocalPosition(position);
            updateTransform(true);
        }

        void SetLocalRotation(const glm::quat &rotation)
        {
            parent ? transform.SetLocalRotationWithParent(parent->transform, rotation) : transform.SetLocalRotation(rotation);
            updateTransform(true);
        }

        void SetLocalScale(const glm::vec3 &scale)
        {
            parent ? transform.SetLocalScaleWithParent(parent->transform, scale) : transform.SetLocalScale(scale);
            updateTransform(true);
        }

        void RotateGlobal(const float det, const glm::vec3 &axis)
        {
            parent ? transform.RotateGlobalWithParent(parent->transform, det, axis) : transform.RotateGlobal(det, axis);
            updateTransform(true);
        }

        void RotateLocal(const float det, const glm::vec3 &axis)
        {
            parent ? transform.RotateLocalWithParent(parent->transform, det, axis) : transform.RotateLocal(det, axis);
            updateTransform(true);
        }

        void TranslateLocal(const glm::vec3 &det)
        {
            parent ? transform.TranslateLocalWithParent(parent->transform.model, det) : transform.TranslateLocal(det);
            updateTransform(true);
        }

        void TranslateGlobal(const glm::vec3 &det)
        {
            parent ? transform.TranslateGlobalWithParent(parent->transform.model, det) : transform.TranslateGlobal(det);
            updateTransform(true);
        }

        void Scale(const glm::vec3 &scale)
        {
            parent ? transform.ScaleWithParent(parent->transform, scale) : transform.Scale(scale);
            updateTransform(true);
        }

    private:
        void init(const nlohmann::json &json, std::map<vke_ds::id64_t, std::unique_ptr<GameObject>> &objects)
        {
            std::string stdname = json["name"];
            memcpy(name, stdname.c_str(), stdname.length() + 1);
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

        void updateTransform(bool first)
        {
            if (!first)
                transform.UpdateWithParent(parent->transform);

            for (auto &component : components)
                component->OnTransformed(transform);

            for (auto &kv : children)
                kv.second->updateTransform(false);
        }

        std::unique_ptr<Component> loadComponent(const nlohmann::json &json);
    };
}

#endif