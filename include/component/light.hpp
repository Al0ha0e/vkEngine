#ifndef COMPONENT_LIGHT_H
#define COMPONENT_LIGHT_H

#include <component/transform.hpp>
#include <render/render.hpp>

namespace vke_component
{
    struct DirectionalLightData
    {
        glm::vec3 color;
        float intensity;
        DirectionalLightData() = default;
        DirectionalLightData(const nlohmann::json &json)
            : color(json["color"][0], json["color"][1], json["color"][2]), intensity(json["intensity"]) {}
        nlohmann::json ToJSON() const
        {
            return {{"type", "directionalLight"}, {"color", {color.r, color.g, color.b}}, {"intensity", intensity}};
        }
    };

    struct PointLightData
    {
        glm::vec3 color;
        float radius;
        float intensity;
        PointLightData() = default;
        PointLightData(const nlohmann::json &json)
            : color(json["color"][0], json["color"][1], json["color"][2]),
              radius(json["radius"]), intensity(json["intensity"]) {}
        nlohmann::json ToJSON() const
        {
            return {{"type", "pointLight"}, {"color", {color.r, color.g, color.b}}, {"radius", radius}, {"intensity", intensity}};
        }
    };

    struct SpotLightData
    {
        glm::vec3 color;
        float radius;
        float intensity;
        float innerConeCos;
        float outerConeCos;
        bool castShadow;
        SpotLightData() = default;
        SpotLightData(const nlohmann::json &json)
            : color(json["color"][0], json["color"][1], json["color"][2]),
              radius(json["radius"]), intensity(json["intensity"]),
              innerConeCos(glm::cos(glm::radians(json["innerCone"].get<float>()))),
              outerConeCos(glm::cos(glm::radians(json["outerCone"].get<float>()))),
              castShadow(json.value("castShadow", json.value("shadowSlot", 0u) != 0u)) {}
        nlohmann::json ToJSON() const
        {
            return {{"type", "spotLight"}, {"color", {color.r, color.g, color.b}}, {"radius", radius}, {"intensity", intensity}, {"innerCone", glm::degrees(glm::acos(glm::clamp(innerConeCos, -1.0f, 1.0f)))}, {"outerCone", glm::degrees(glm::acos(glm::clamp(outerConeCos, -1.0f, 1.0f)))}, {"castShadow", castShadow}};
        }
    };

    class DirectionalLight
    {
    public:
        vke_render::DirectionalLight light;

        DirectionalLight() {}

        DirectionalLight(const vke_common::Transform &transform,
                         const DirectionalLightData &componentData)
        {
            glm::vec3 forward =
                transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
            light = vke_render::DirectionalLight(
                glm::vec4(glm::normalize(forward), 0.0f),
                glm::vec4(componentData.color, componentData.intensity));
        }

        void FillData(DirectionalLightData &data) const
        {
            data.color = glm::vec3(light.colorWithIntensity);
            data.intensity = light.colorWithIntensity.w;
        }

        void LoadToEngine(entt::entity entity)
        {
            vke_render::Renderer::GetInstance()->lightManager->AddLight<vke_render::DirectionalLight>(entity, light);
        }
    };

    class PointLight
    {
    public:
        vke_render::PointLight light;

        PointLight() {}

        PointLight(const vke_common::Transform &transform,
                   const PointLightData &componentData)
        {
            light = vke_render::PointLight(
                glm::vec4(transform.GetGlobalPosition(), componentData.radius),
                glm::vec4(componentData.color, componentData.intensity));
        }

        void FillData(PointLightData &data) const
        {
            data.color = glm::vec3(light.colorWithIntensity);
            data.radius = light.positionWithRadius.w;
            data.intensity = light.colorWithIntensity.w;
        }

        void LoadToEngine(entt::entity entity)
        {
            vke_render::Renderer::GetInstance()->lightManager->AddLight<vke_render::PointLight>(entity, light);
        }
    };

    class SpotLight
    {
    public:
        vke_render::SpotLight light;

        SpotLight() {}

        SpotLight(const vke_common::Transform &transform,
                  const SpotLightData &componentData)
        {
            glm::vec3 forward =
                transform.GetGlobalRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
            light = vke_render::SpotLight(
                glm::vec4(transform.GetGlobalPosition(), componentData.radius),
                glm::vec4(glm::normalize(forward), 0.0f),
                glm::vec4(componentData.color, componentData.intensity),
                glm::vec4(componentData.innerConeCos, componentData.outerConeCos,
                          componentData.castShadow ? 1.0f : 0.0f, 0.0f));
        }

        void FillData(SpotLightData &data) const
        {
            data.color = glm::vec3(light.colorWithIntensity);
            data.radius = light.positionWithRadius.w;
            data.intensity = light.colorWithIntensity.w;
            data.innerConeCos = light.cone.x;
            data.outerConeCos = light.cone.y;
            data.castShadow = light.CastShadow();
        }

        void LoadToEngine(entt::entity entity)
        {
            vke_render::Renderer::GetInstance()->lightManager->AddLight<vke_render::SpotLight>(entity, light);
        }
    };
}

#endif
