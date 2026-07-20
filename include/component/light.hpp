#ifndef COMPONENT_LIGHT_H
#define COMPONENT_LIGHT_H

#include <component/transform.hpp>
#include <render/render.hpp>

namespace vke_component
{
    class DirectionalLight
    {
    public:
        vke_render::DirectionalLight light;

        DirectionalLight() {}

        DirectionalLight(const vke_common::Transform &transform, const nlohmann::json &json)
        {
            auto &color = json["color"];
            float intensity = json["intensity"];
            glm::vec3 forward = transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
            light = vke_render::DirectionalLight(
                glm::vec4(glm::normalize(forward), 0.0f),
                glm::vec4(color[0], color[1], color[2], intensity));
        }

        void LoadToEngine(entt::entity entity)
        {
            vke_render::Renderer::GetInstance()->lightManager->AddLight<vke_render::DirectionalLight>(entity, light);
        }

        nlohmann::json ToJSON() { return light.ToJSON(); }
    };

    class PointLight
    {
    public:
        vke_render::PointLight light;

        PointLight() {}

        PointLight(const vke_common::Transform &transform, const nlohmann::json &json)
        {
            auto &color = json["color"];
            float radius = json["radius"];
            float intensity = json["intensity"];
            light = vke_render::PointLight(
                glm::vec4(transform.GetGlobalPosition(), radius),
                glm::vec4(color[0], color[1], color[2], intensity));
        }

        void LoadToEngine(entt::entity entity)
        {
            vke_render::Renderer::GetInstance()->lightManager->AddLight<vke_render::PointLight>(entity, light);
        }

        nlohmann::json ToJSON() { return light.ToJSON(); }
    };

    class SpotLight
    {
    public:
        vke_render::SpotLight light;

        SpotLight() {}

        SpotLight(const vke_common::Transform &transform, const nlohmann::json &json)
        {
            auto &color = json["color"];
            float radius = json["radius"];
            float intensity = json["intensity"];
            float innerCone = glm::radians(json["innerCone"].get<float>());
            float outerCone = glm::radians(json["outerCone"].get<float>());
            bool castShadow = json.value("castShadow", json.value("shadowSlot", 0u) != 0u);
            glm::vec3 forward = transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
            light = vke_render::SpotLight(
                glm::vec4(transform.GetGlobalPosition(), radius),
                glm::vec4(glm::normalize(forward), 0.0f),
                glm::vec4(color[0], color[1], color[2], intensity),
                glm::vec4(glm::cos(innerCone), glm::cos(outerCone), castShadow ? 1.0f : 0.0f, 0.0f));
        }

        void LoadToEngine(entt::entity entity)
        {
            vke_render::Renderer::GetInstance()->lightManager->AddLight<vke_render::SpotLight>(entity, light);
        }

        nlohmann::json ToJSON() { return light.ToJSON(); }
    };
}

#endif