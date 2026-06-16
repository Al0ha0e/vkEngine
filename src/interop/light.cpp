#include <interop/light.hpp>
#include <render/render.hpp>
#include <scene.hpp>
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

namespace vke_interop
{
    static inline entt::entity ToEntity(uint32_t entity)
    {
        return static_cast<entt::entity>(entity);
    }

    template <vke_render::AllowedLightType T>
    static T &GetLightWithoutCheck(uint32_t entity)
    {
        return vke_render::Renderer::GetInstance()->lightManager->GetLightWithoutCheck<T>(ToEntity(entity));
    }

    template <vke_render::AllowedLightType T>
    static void MarkLightDirty()
    {
        vke_render::Renderer::GetInstance()->lightManager->MarkDirty<T>();
    }

    static Vector3<float> ToInteropColor(const glm::vec4 &colorWithIntensity)
    {
        return Vector3<float>{colorWithIntensity.x, colorWithIntensity.y, colorWithIntensity.z};
    }

    static glm::vec3 ToGlm(const Vector3<float> &value)
    {
        return glm::vec3(value.x, value.y, value.z);
    }

    template <vke_render::AllowedLightType T>
    static void GetLightColor(uint32_t entity, Vector3<float> *color)
    {
        *color = ToInteropColor(GetLightWithoutCheck<T>(entity).colorWithIntensity);
    }

    template <vke_render::AllowedLightType T>
    static void SetLightColor(uint32_t entity, const Vector3<float> *color)
    {
        auto &light = GetLightWithoutCheck<T>(entity);
        glm::vec3 rgb = ToGlm(*color);
        light.colorWithIntensity = glm::vec4(rgb, light.colorWithIntensity.w);
        MarkLightDirty<T>();
    }

    template <vke_render::AllowedLightType T>
    static float GetLightIntensity(uint32_t entity)
    {
        return GetLightWithoutCheck<T>(entity).colorWithIntensity.w;
    }

    template <vke_render::AllowedLightType T>
    static void SetLightIntensity(uint32_t entity, float intensity)
    {
        GetLightWithoutCheck<T>(entity).colorWithIntensity.w = intensity;
        MarkLightDirty<T>();
    }

    void VKE_INTEROP_CDECL GetDirectionalLightColor(uint32_t entity, Vector3<float> *color)
    {
        GetLightColor<vke_render::DirectionalLight>(entity, color);
    }

    void VKE_INTEROP_CDECL SetDirectionalLightColor(uint32_t entity, const Vector3<float> *color)
    {
        SetLightColor<vke_render::DirectionalLight>(entity, color);
    }

    float VKE_INTEROP_CDECL GetDirectionalLightIntensity(uint32_t entity)
    {
        return GetLightIntensity<vke_render::DirectionalLight>(entity);
    }

    void VKE_INTEROP_CDECL SetDirectionalLightIntensity(uint32_t entity, float intensity)
    {
        SetLightIntensity<vke_render::DirectionalLight>(entity, intensity);
    }

    void VKE_INTEROP_CDECL GetPointLightColor(uint32_t entity, Vector3<float> *color)
    {
        GetLightColor<vke_render::PointLight>(entity, color);
    }

    void VKE_INTEROP_CDECL SetPointLightColor(uint32_t entity, const Vector3<float> *color)
    {
        SetLightColor<vke_render::PointLight>(entity, color);
    }

    float VKE_INTEROP_CDECL GetPointLightIntensity(uint32_t entity)
    {
        return GetLightIntensity<vke_render::PointLight>(entity);
    }

    void VKE_INTEROP_CDECL SetPointLightIntensity(uint32_t entity, float intensity)
    {
        SetLightIntensity<vke_render::PointLight>(entity, intensity);
    }

    float VKE_INTEROP_CDECL GetPointLightRadius(uint32_t entity)
    {
        return GetLightWithoutCheck<vke_render::PointLight>(entity).positionWithRadius.w;
    }

    void VKE_INTEROP_CDECL SetPointLightRadius(uint32_t entity, float radius)
    {
        GetLightWithoutCheck<vke_render::PointLight>(entity).positionWithRadius.w = radius;
        MarkLightDirty<vke_render::PointLight>();
    }

    void VKE_INTEROP_CDECL GetSpotLightColor(uint32_t entity, Vector3<float> *color)
    {
        GetLightColor<vke_render::SpotLight>(entity, color);
    }

    void VKE_INTEROP_CDECL SetSpotLightColor(uint32_t entity, const Vector3<float> *color)
    {
        SetLightColor<vke_render::SpotLight>(entity, color);
    }

    float VKE_INTEROP_CDECL GetSpotLightIntensity(uint32_t entity)
    {
        return GetLightIntensity<vke_render::SpotLight>(entity);
    }

    void VKE_INTEROP_CDECL SetSpotLightIntensity(uint32_t entity, float intensity)
    {
        SetLightIntensity<vke_render::SpotLight>(entity, intensity);
    }

    float VKE_INTEROP_CDECL GetSpotLightRadius(uint32_t entity)
    {
        return GetLightWithoutCheck<vke_render::SpotLight>(entity).positionWithRadius.w;
    }

    void VKE_INTEROP_CDECL SetSpotLightRadius(uint32_t entity, float radius)
    {
        GetLightWithoutCheck<vke_render::SpotLight>(entity).positionWithRadius.w = radius;
        MarkLightDirty<vke_render::SpotLight>();
    }

    float VKE_INTEROP_CDECL GetSpotLightInnerCone(uint32_t entity)
    {
        const float cosValue = GetLightWithoutCheck<vke_render::SpotLight>(entity).cone.x;
        return glm::acos(glm::clamp(cosValue, -1.0f, 1.0f));
    }

    void VKE_INTEROP_CDECL SetSpotLightInnerCone(uint32_t entity, float radians)
    {
        GetLightWithoutCheck<vke_render::SpotLight>(entity).cone.x = glm::cos(radians);
        MarkLightDirty<vke_render::SpotLight>();
    }

    float VKE_INTEROP_CDECL GetSpotLightOuterCone(uint32_t entity)
    {
        const float cosValue = GetLightWithoutCheck<vke_render::SpotLight>(entity).cone.y;
        return glm::acos(glm::clamp(cosValue, -1.0f, 1.0f));
    }

    void VKE_INTEROP_CDECL SetSpotLightOuterCone(uint32_t entity, float radians)
    {
        GetLightWithoutCheck<vke_render::SpotLight>(entity).cone.y = glm::cos(radians);
        MarkLightDirty<vke_render::SpotLight>();
    }
}
