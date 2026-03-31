#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include <render/render.hpp>
#include <component/transform.hpp>

namespace vke_component
{
    inline glm::vec3 TransformForward(const vke_common::Transform &transform)
    {
        const glm::quat rotation = transform.GetGlobalRotation();
        return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    template <vke_render::AllowedLightType T>
    class LightComponent
    {
    public:
        vke_ds::id32_t id;
        LightComponent() : id(0) {}
    };

    class DirectionalLightComponent : public LightComponent<vke_render::DirectionalLight>
    {
    public:
        DirectionalLightComponent() : LightComponent<vke_render::DirectionalLight>() {}
        DirectionalLightComponent(const vke_common::Transform &transform,
                                  const nlohmann::json &json)
            : LightComponent<vke_render::DirectionalLight>()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &dir = json["dir"];
            auto &color = json["color"];
            float intensity = json["intensity"];
            id = lightManager->AddLight<vke_render::DirectionalLight>(
                glm::vec4(dir[0], dir[1], dir[2], 0),
                glm::vec4(color[0], color[1], color[2], intensity));
            VKE_FATAL_IF(id >= vke_render::MAX_DIRECTIONAL_LIGHT_CNT, "NO MORE DIRECTIONAL_LIGHT")
        }

        glm::vec3 GetDirection() const
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            return glm::vec3(lightManager->GetLight<vke_render::DirectionalLight>(id).direction);
        }

        glm::vec3 GetColor() const
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            return glm::vec3(lightManager->GetLight<vke_render::DirectionalLight>(id).colorWithIntensity);
        }

        float GetIntensity() const
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            return lightManager->GetLight<vke_render::DirectionalLight>(id).colorWithIntensity.w;
        }

        void OnTransformed(const vke_common::Transform &transform)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::DirectionalLight>(id);
            light.direction = glm::vec4(glm::normalize(TransformForward(transform)), 0.0f);
            lightManager->MarkDirty<vke_render::DirectionalLight>();
        }

        void SetDirection(const glm::vec3 &dir)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::DirectionalLight>(id);
            light.direction = glm::vec4(dir, 0.0f);
            lightManager->MarkDirty<vke_render::DirectionalLight>();
        }

        void SetColor(const glm::vec3 &color)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::DirectionalLight>(id);
            light.colorWithIntensity = glm::vec4(color, light.colorWithIntensity.w);
            lightManager->MarkDirty<vke_render::DirectionalLight>();
        }

        void SetIntensity(const float intensity)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::DirectionalLight>(id);
            light.colorWithIntensity.w = intensity;
            lightManager->MarkDirty<vke_render::DirectionalLight>();
        }

        nlohmann::json ToJSON() const
        {
            nlohmann::json j;
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::DirectionalLight>(id);
            j["dir"] = {light.direction.x, light.direction.y, light.direction.z};
            j["color"] = {light.colorWithIntensity.x, light.colorWithIntensity.y, light.colorWithIntensity.z};
            j["intensity"] = light.colorWithIntensity.w;
            return j;
        }
    };

    class PointLightComponent : LightComponent<vke_render::PointLight>
    {
    public:
        PointLightComponent() : LightComponent<vke_render::PointLight>() {}

        PointLightComponent(const vke_common::Transform &transform,
                            const nlohmann::json &json)
            : LightComponent<vke_render::PointLight>()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();

            auto &color = json["color"];
            float radius = json["radius"];
            float intensity = json["intensity"];

            id = lightManager->AddLight<vke_render::PointLight>(
                glm::vec4(transform.GetGlobalPosition(), radius),
                glm::vec4(color[0], color[1], color[2], intensity));

            VKE_FATAL_IF(id >= vke_render::MAX_POINT_LIGHT_CNT,
                         "NO MORE POINT_LIGHT");
        }

        glm::vec3 GetPosition() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            return glm::vec3(light.positionWithRadius);
        }

        float GetRadius() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            return light.positionWithRadius.w;
        }

        glm::vec3 GetColor() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            return glm::vec3(light.colorWithIntensity);
        }

        float GetIntensity() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            return light.colorWithIntensity.w;
        }

        void OnTransformed(const vke_common::Transform &transform)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::PointLight>(id);
            light.positionWithRadius = glm::vec4(transform.GetGlobalPosition(), light.positionWithRadius.w);
            lightManager->MarkDirty<vke_render::PointLight>();
        }

        void SetPosition(const glm::vec3 &pos)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            light.positionWithRadius = glm::vec4(pos, light.positionWithRadius.w);
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::PointLight>();
        }

        void SetRadius(float radius)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            light.positionWithRadius.w = radius;
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::PointLight>();
        }

        void SetColor(const glm::vec3 &color)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            light.colorWithIntensity = glm::vec4(color, light.colorWithIntensity.w);
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::PointLight>();
        }

        void SetIntensity(float intensity)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            light.colorWithIntensity.w = intensity;
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::PointLight>();
        }

        nlohmann::json ToJSON() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::PointLight>(id);
            nlohmann::json j;
            j["position"] = {light.positionWithRadius.x, light.positionWithRadius.y, light.positionWithRadius.z};
            j["radius"] = light.positionWithRadius.w;
            j["color"] = {light.colorWithIntensity.x, light.colorWithIntensity.y, light.colorWithIntensity.z};
            j["intensity"] = light.colorWithIntensity.w;
            return j;
        }
    };

    class SpotLightComponent : public LightComponent<vke_render::SpotLight>
    {
    public:
        SpotLightComponent() : LightComponent<vke_render::SpotLight>() {}

        SpotLightComponent(const vke_common::Transform &transform,
                           const nlohmann::json &json)
            : LightComponent<vke_render::SpotLight>()
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();

            auto &dir = json["dir"];
            auto &color = json["color"];
            float radius = json["radius"];
            float intensity = json["intensity"];
            float innerCone = glm::radians(json["innerCone"].get<float>());
            float outerCone = glm::radians(json["outerCone"].get<float>());

            id = lightManager->AddLight<vke_render::SpotLight>(
                glm::vec4(transform.GetGlobalPosition(), radius),
                glm::vec4(dir[0], dir[1], dir[2], 0.0f),
                glm::vec4(color[0], color[1], color[2], intensity),
                glm::vec4(glm::cos(innerCone), glm::cos(outerCone), 0.0f, 0.0f));

            VKE_FATAL_IF(id >= vke_render::MAX_SPOT_LIGHT_CNT,
                         "NO MORE SPOT_LIGHT");
        }

        glm::vec3 GetPosition() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return glm::vec3(light.positionWithRadius);
        }

        float GetRadius() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return light.positionWithRadius.w;
        }

        void SetPosition(const glm::vec3 &pos)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.positionWithRadius = glm::vec4(pos, light.positionWithRadius.w);
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        glm::vec3 GetDirection() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return glm::vec3(light.direction);
        }

        glm::vec3 GetColor() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return glm::vec3(light.colorWithIntensity);
        }

        float GetIntensity() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return light.colorWithIntensity.w;
        }

        float GetInnerCone() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return light.cone.x;
        }

        float GetOuterCone() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            return light.cone.y;
        }

        void OnTransformed(const vke_common::Transform &transform)
        {
            vke_render::Renderer *renderer = vke_render::Renderer::GetInstance();
            vke_render::LightManager *lightManager = renderer->lightManager.get();
            auto &light = lightManager->GetLight<vke_render::SpotLight>(id);
            light.positionWithRadius = glm::vec4(transform.GetGlobalPosition(), light.positionWithRadius.w);
            light.direction = glm::vec4(glm::normalize(TransformForward(transform)), 0.0f);
            lightManager->MarkDirty<vke_render::SpotLight>();
        }

        void SetRadius(float radius)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.positionWithRadius.w = radius;
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        void SetDirection(const glm::vec3 &dir)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.direction = glm::vec4(dir, 0.0f);
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        void SetColor(const glm::vec3 &color)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.colorWithIntensity = glm::vec4(color, light.colorWithIntensity.w);
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        void SetIntensity(float intensity)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.colorWithIntensity.w = intensity;
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        void SetInnerCone(float inner)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.cone.x = inner;
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        void SetOuterCone(float outer)
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            light.cone.y = outer;
            vke_render::Renderer::GetInstance()->lightManager->MarkDirty<vke_render::SpotLight>();
        }

        nlohmann::json ToJSON() const
        {
            auto &light = vke_render::Renderer::GetInstance()->lightManager->GetLight<vke_render::SpotLight>(id);
            nlohmann::json j;
            j["position"] = {light.positionWithRadius.x, light.positionWithRadius.y, light.positionWithRadius.z};
            j["radius"] = light.positionWithRadius.w;
            j["dir"] = {light.direction.x, light.direction.y, light.direction.z};
            j["color"] = {light.colorWithIntensity.x, light.colorWithIntensity.y, light.colorWithIntensity.z};
            j["intensity"] = light.colorWithIntensity.w;
            j["innerCone"] = glm::degrees(light.cone.x);
            j["outerCone"] = glm::degrees(light.cone.y);
            return j;
        }
    };
}

#endif
