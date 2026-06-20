#ifndef LIGHT_H
#define LIGHT_H

#include <render/buffer.hpp>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <concepts>
#include <type_traits>
#include <memory>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace vke_render
{
    enum class LightType
    {
        DIRECTIONAL_LIGHT,
        POINT_LIGHT,
        SPOT_LIGHT,
        LIGHT_TYPE_CNT
    };

    struct DirectionalLight
    {
        static constexpr LightType type = LightType::DIRECTIONAL_LIGHT;
        glm::vec4 direction;          // xyz
        glm::vec4 colorWithIntensity; // w: intensity

        DirectionalLight() : direction(1, 0, 0, 0), colorWithIntensity(0) {}
        DirectionalLight(glm::vec4 direction, glm::vec4 color) : direction(direction), colorWithIntensity(color) {}

        nlohmann::json ToJSON() const
        {
            return {
                {"type", "directionalLight"},
                {"color", {colorWithIntensity.x, colorWithIntensity.y, colorWithIntensity.z}},
                {"intensity", colorWithIntensity.w}};
        }
    };

    struct PointLight
    {
        static constexpr LightType type = LightType::POINT_LIGHT;
        glm::vec4 positionWithRadius; // w: radius
        glm::vec4 colorWithIntensity; // w: intensity

        PointLight() : positionWithRadius(0), colorWithIntensity(0) {}
        PointLight(glm::vec4 positionWithRadius, glm::vec4 colorWithIntensity)
            : positionWithRadius(positionWithRadius), colorWithIntensity(colorWithIntensity) {}

        nlohmann::json ToJSON() const
        {
            return {
                {"type", "pointLight"},
                {"radius", positionWithRadius.w},
                {"color", {colorWithIntensity.x, colorWithIntensity.y, colorWithIntensity.z}},
                {"intensity", colorWithIntensity.w}};
        }
    };

    struct SpotLight
    {
        static constexpr LightType type = LightType::SPOT_LIGHT;
        glm::vec4 positionWithRadius; // w: radius
        glm::vec4 direction;          // xyz
        glm::vec4 colorWithIntensity; // w: intensity
        glm::vec4 cone;               // x:inner, y: outer, z: shadow slot + 1 (0 disables shadow)

        SpotLight() : positionWithRadius(0), direction(1, 0, 0, 0), colorWithIntensity(0), cone(0) {}
        SpotLight(glm::vec4 positionWithRadius, glm::vec4 direction, glm::vec4 colorWithIntensity, glm::vec4 cone)
            : positionWithRadius(positionWithRadius), direction(direction), colorWithIntensity(colorWithIntensity), cone(cone) {}

        nlohmann::json ToJSON() const
        {
            return {
                {"type", "spotLight"},
                {"radius", positionWithRadius.w},
                {"color", {colorWithIntensity.x, colorWithIntensity.y, colorWithIntensity.z}},
                {"intensity", colorWithIntensity.w},
                {"innerCone", glm::degrees(glm::acos(glm::clamp(cone.x, -1.0f, 1.0f)))},
                {"outerCone", glm::degrees(glm::acos(glm::clamp(cone.y, -1.0f, 1.0f)))},
                {"castShadow", CastShadow()}};
        }

        bool CastShadow() const { return cone.z != 0.0f; }
        int GetShadowSlot() { return (int)cone.z - 1; }
        void SetShadowSlot(bool castShadow, int slot) { cone.z = castShadow ? slot + 1 : 0; }
    };

    template <typename T>
    concept AllowedLightType = std::same_as<T, DirectionalLight> || std::same_as<T, PointLight> || std::same_as<T, SpotLight>;

    const uint32_t MAX_DIRECTIONAL_LIGHT_CNT = 4;
    const uint32_t MAX_POINT_LIGHT_CNT = 1024;
    const uint32_t MAX_SPOT_LIGHT_CNT = 512;
    const uint32_t MAX_LIGHT_PER_CLUSTER = 64;
    const uint32_t CLUSTER_DIM_X = 16;
    const uint32_t CLUSTER_DIM_Y = 8;
    const uint32_t CLUSTER_DIM_Z = 16;

    constexpr uint32_t MAX_LIGHT_CNTS[] = {MAX_DIRECTIONAL_LIGHT_CNT, MAX_POINT_LIGHT_CNT, MAX_SPOT_LIGHT_CNT};
    constexpr size_t LIGHT_SIZES[] = {sizeof(DirectionalLight), sizeof(PointLight), sizeof(SpotLight)};
    constexpr uint32_t LIGHT_MAP_ST[] = {0, MAX_DIRECTIONAL_LIGHT_CNT, MAX_DIRECTIONAL_LIGHT_CNT + MAX_POINT_LIGHT_CNT,
                                         MAX_DIRECTIONAL_LIGHT_CNT + MAX_POINT_LIGHT_CNT + MAX_SPOT_LIGHT_CNT};

    struct CPULightData
    {
        std::unique_ptr<HostCoherentBuffer> cpuLightBuffers[(int)LightType::LIGHT_TYPE_CNT];
        uint32_t lightCnts[(int)LightType::LIGHT_TYPE_CNT];
        std::unordered_map<entt::entity, vke_ds::id32_t> entityToLight[(int)LightType::LIGHT_TYPE_CNT];
        std::vector<entt::entity> ownerMaps[(int)LightType::LIGHT_TYPE_CNT];

        CPULightData()
        {
            for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
            {
                lightCnts[i] = 0;
                cpuLightBuffers[i] = std::make_unique<HostCoherentBuffer>(LIGHT_SIZES[i] * MAX_LIGHT_CNTS[i],
                                                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            }
        }

        CPULightData(const CPULightData &) = delete;
        CPULightData &operator=(const CPULightData &) = delete;
        CPULightData(CPULightData &&) = default;
        CPULightData &operator=(CPULightData &&) = default;

        void Clear()
        {
            for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
            {
                lightCnts[i] = 0;
                entityToLight[i].clear();
                ownerMaps[i].clear();
            }
        }

        template <AllowedLightType T>
        bool HasLight(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            return entityToLight[typecode].find(entity) != entityToLight[typecode].end();
        }
    };

    struct SceneLightData
    {
        std::shared_ptr<CPULightData> cpuLightData;

        SceneLightData()
            : cpuLightData(std::make_shared<CPULightData>()) {}
        explicit SceneLightData(std::shared_ptr<CPULightData> data)
            : cpuLightData(std::move(data))
        {
            if (cpuLightData == nullptr)
                cpuLightData = std::make_shared<CPULightData>();
        }
        SceneLightData(const SceneLightData &) = default;
        SceneLightData &operator=(const SceneLightData &) = default;
        SceneLightData(SceneLightData &&) = default;
        SceneLightData &operator=(SceneLightData &&) = default;

        template <AllowedLightType T>
        T *GetLightBuffer()
        {
            return reinterpret_cast<T *>(cpuLightData->cpuLightBuffers[(int)T::type]->data);
        }

        template <AllowedLightType T>
        const T *GetLightBuffer() const
        {
            return reinterpret_cast<const T *>(cpuLightData->cpuLightBuffers[(int)T::type]->data);
        }

        template <AllowedLightType T>
        void AppendLight(entt::entity entity, const T &light)
        {
            const int typecode = (int)T::type;
            uint32_t &cnt = cpuLightData->lightCnts[typecode];
            VKE_FATAL_IF(cnt >= MAX_LIGHT_CNTS[typecode], "NO MORE LIGHT OF TYPE {}", typecode)
            cpuLightData->entityToLight[typecode][entity] = static_cast<vke_ds::id32_t>(cnt);
            cpuLightData->ownerMaps[typecode].push_back(entity);
            std::construct_at(GetLightBuffer<T>() + cnt, light);
            ++cnt;
        }

        template <AllowedLightType T>
        bool HasLight(entt::entity entity) const
        {
            return cpuLightData->HasLight<T>(entity);
        }

        template <AllowedLightType T>
        const T &GetLightWithoutCheckByEntity(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            const vke_ds::id32_t id = cpuLightData->entityToLight[typecode].at(entity);
            return GetLightBuffer<T>()[id];
        }
    };
}

#endif
