#ifndef INTEROP_LIGHT_H
#define INTEROP_LIGHT_H

#include <cstdint>
#include <interop/interop.hpp>
#include <interop/math.hpp>

namespace vke_interop
{
    using GetDirectionalLightColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector3<float> *);
    using SetDirectionalLightColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using GetDirectionalLightIntensityFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetDirectionalLightIntensityFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);

    using GetPointLightColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector3<float> *);
    using SetPointLightColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using GetPointLightIntensityFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetPointLightIntensityFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using GetPointLightRadiusFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetPointLightRadiusFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);

    using GetSpotLightColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, Vector3<float> *);
    using SetSpotLightColorFn = void(VKE_INTEROP_CDECL *)(uint32_t, const Vector3<float> *);
    using GetSpotLightIntensityFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetSpotLightIntensityFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using GetSpotLightRadiusFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetSpotLightRadiusFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using GetSpotLightInnerConeFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetSpotLightInnerConeFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);
    using GetSpotLightOuterConeFn = float(VKE_INTEROP_CDECL *)(uint32_t);
    using SetSpotLightOuterConeFn = void(VKE_INTEROP_CDECL *)(uint32_t, float);

    void VKE_INTEROP_CDECL GetDirectionalLightColor(uint32_t entity, Vector3<float> *color);
    void VKE_INTEROP_CDECL SetDirectionalLightColor(uint32_t entity, const Vector3<float> *color);
    float VKE_INTEROP_CDECL GetDirectionalLightIntensity(uint32_t entity);
    void VKE_INTEROP_CDECL SetDirectionalLightIntensity(uint32_t entity, float intensity);

    void VKE_INTEROP_CDECL GetPointLightColor(uint32_t entity, Vector3<float> *color);
    void VKE_INTEROP_CDECL SetPointLightColor(uint32_t entity, const Vector3<float> *color);
    float VKE_INTEROP_CDECL GetPointLightIntensity(uint32_t entity);
    void VKE_INTEROP_CDECL SetPointLightIntensity(uint32_t entity, float intensity);
    float VKE_INTEROP_CDECL GetPointLightRadius(uint32_t entity);
    void VKE_INTEROP_CDECL SetPointLightRadius(uint32_t entity, float radius);

    void VKE_INTEROP_CDECL GetSpotLightColor(uint32_t entity, Vector3<float> *color);
    void VKE_INTEROP_CDECL SetSpotLightColor(uint32_t entity, const Vector3<float> *color);
    float VKE_INTEROP_CDECL GetSpotLightIntensity(uint32_t entity);
    void VKE_INTEROP_CDECL SetSpotLightIntensity(uint32_t entity, float intensity);
    float VKE_INTEROP_CDECL GetSpotLightRadius(uint32_t entity);
    void VKE_INTEROP_CDECL SetSpotLightRadius(uint32_t entity, float radius);
    float VKE_INTEROP_CDECL GetSpotLightInnerCone(uint32_t entity);
    void VKE_INTEROP_CDECL SetSpotLightInnerCone(uint32_t entity, float radians);
    float VKE_INTEROP_CDECL GetSpotLightOuterCone(uint32_t entity);
    void VKE_INTEROP_CDECL SetSpotLightOuterCone(uint32_t entity, float radians);
}

#endif
