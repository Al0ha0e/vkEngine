#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <entt/entity/entity.hpp>
#include <set>

namespace vke_common
{
    struct Transform
    {
        glm::mat4 model;
        glm::quat localRotation;
        glm::vec3 localPosition;
        glm::vec3 localScale;
        entt::entity parent = entt::null;
        std::set<entt::entity> children;

        Transform()
            : model(1), localPosition(0), localScale(1), localRotation(glm::vec3(0))
        {
            init();
        }

        Transform(glm::vec3 pos, glm::vec3 scl, glm::quat rot)
            : localPosition(pos), localScale(scl), localRotation(rot)
        {
            init();
        }

        Transform(const Transform &fa, glm::vec3 pos, glm::vec3 scl, glm::quat rot)
            : localPosition(pos), localScale(scl), localRotation(rot)
        {
            initWithParent(fa);
        }

        Transform(const nlohmann::json &json)
        {
            auto pos = json["pos"];
            auto scl = json["scl"];
            auto rot = json["rot"];
            localPosition = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
            localScale = glm::vec3(scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>());
            localRotation = glm::normalize(glm::quat(rot[3].get<float>(), rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>()));
            init();
        }

        Transform(const Transform &fa, const nlohmann::json &json)
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
            if (scale.x == 0 || scale.y == 0 || scale.z == 0)
                return glm::quat(1, 0, 0, 0);
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

        void SetParent(const Transform &fa)
        {
            glm::mat4 inv(glm::inverse(fa.model));

            localPosition = inv * glm::vec4(GetGlobalPosition(), 1);
            localRotation = glm::normalize(glm::inverse(fa.GetGlobalRotation()) * GetGlobalRotation());
            localScale = GetLossyScale() / fa.GetLossyScale();
            calcModelMatrixWithParent(fa.model);
        }

        void SetParentFixedLocal(const Transform &fa)
        {
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

        void SetLocalRotationWithParent(const Transform &fa, const glm::quat &rot)
        {
            localRotation = glm::normalize(rot);
            calcModelMatrixWithParent(fa.model);
        }

        void RotateLocal(const float det, const glm::vec3 &axis)
        {
            localRotation = glm::normalize(glm::rotate(localRotation, det, axis));
            calcModelMatrix();
        }

        void RotateLocalWithParent(const Transform &fa, const float det, const glm::vec3 &axis)
        {
            localRotation = glm::normalize(glm::rotate(localRotation, det, axis));
            calcModelMatrixWithParent(fa.model);
        }

        void RotateGlobal(const float det, const glm::vec3 &axis)
        {
            localRotation = glm::normalize(glm::angleAxis(det, glm::normalize(axis)) * localRotation);
            calcModelMatrix();
        }

        void RotateGlobalWithParent(const Transform &fa, const float det, const glm::vec3 &axis)
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

        void SetLocalScaleWithParent(const Transform &fa, const glm::vec3 &scale)
        {
            localScale = scale;
            calcModelMatrixWithParent(fa.model);
        }

        void Scale(const glm::vec3 &scale)
        {
            localScale *= scale;
            calcModelMatrix();
        }

        void ScaleWithParent(const Transform &fa, const glm::vec3 &scale)
        {
            localScale *= scale;
            calcModelMatrixWithParent(fa.model);
        }

        nlohmann::json ToJSON()
        {
            nlohmann::json ret;
            ret["pos"] = {localPosition[0], localPosition[1], localPosition[2]};
            ret["scl"] = {localScale[0], localScale[1], localScale[2]};
            ret["rot"] = {localRotation[0], localRotation[1], localRotation[2], localRotation[3]};
            return ret;
        }

        void UpdateWithParent(const Transform &fa)
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

        void initWithParent(const Transform &fa)
        {
            calcModelMatrixWithParent(fa.model);
        }
    };
}

#endif