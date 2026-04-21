#ifndef LIGHT_H
#define LIGHT_H

#include <render/buffer.hpp>
#include <render/descriptor.hpp>
#include <render/pipeline.hpp>
#include <render/frame_graph.hpp>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <concepts>
#include <type_traits>
#include <memory>

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
        glm::vec4 cone;               // x:inner, y: outer

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
                {"outerCone", glm::degrees(glm::acos(glm::clamp(cone.y, -1.0f, 1.0f)))}};
        }
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

    class SceneLighting
    {
    public:
        bool dirtyFlags[(int)LightType::LIGHT_TYPE_CNT];
        uint32_t lightCnts[(int)LightType::LIGHT_TYPE_CNT];
        std::unordered_map<entt::entity, vke_ds::id32_t> lightMaps[(int)LightType::LIGHT_TYPE_CNT];
        std::unique_ptr<HostCoherentBuffer> cpuLightBuffers[(int)LightType::LIGHT_TYPE_CNT];

        SceneLighting()
        {
            for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
            {
                dirtyFlags[i] = 0;
                lightCnts[i] = 0;
                cpuLightBuffers[i] = std::make_unique<HostCoherentBuffer>(LIGHT_SIZES[i] * MAX_LIGHT_CNTS[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            }
        }

        SceneLighting(const SceneLighting &) = delete;
        SceneLighting &operator=(const SceneLighting &) = delete;
        SceneLighting(SceneLighting &&) = delete;
        SceneLighting &operator=(SceneLighting &&) = delete;

        template <AllowedLightType T, typename... Args>
        void AddLight(entt::entity entity, Args &&...args)
        {
            const int typecode = (int)T::type;
            uint32_t &cnt = lightCnts[typecode];
            VKE_FATAL_IF(cnt >= MAX_LIGHT_CNTS[typecode], "NO MORE LIGHT OF TYPE {}", typecode)

            const vke_ds::id32_t id = cnt++;
            ownerMaps[typecode].push_back(entity);
            std::construct_at(reinterpret_cast<T *>(cpuLightBuffers[typecode]->data) + id, std::forward<Args>(args)...);

            lightMaps[typecode][entity] = id;
            dirtyFlags[typecode] = true;
        }

        template <AllowedLightType T>
        bool HasLight(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            return lightMaps[typecode].find(entity) != lightMaps[typecode].end();
        }

        template <AllowedLightType T>
        T &GetLight(entt::entity entity)
        {
            const int typecode = (int)T::type;
            vke_ds::id32_t id = lightMaps[typecode][entity];
            VKE_FATAL_IF(id >= lightCnts[typecode], "LIGHT NOT EXIST")
            return reinterpret_cast<T *>(cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        void RemoveLight(entt::entity entity)
        {
            const int typecode = (int)T::type;
            auto &lightMap = lightMaps[typecode];
            auto it = lightMap.find(entity);
            if (it == lightMap.end())
                return;

            vke_ds::id32_t id = it->second;
            uint32_t &cnt = lightCnts[typecode];
            VKE_FATAL_IF(id >= cnt, "LIGHT NOT EXIST")

            const uint32_t last = cnt - 1;
            if (id != last)
            {
                const size_t size = LIGHT_SIZES[typecode];
                char *base = reinterpret_cast<char *>(cpuLightBuffers[typecode]->data);
                std::memcpy(base + id * size, base + last * size, size);

                entt::entity swappedOwner = ownerMaps[typecode][last];
                ownerMaps[typecode][id] = swappedOwner;
                lightMap[swappedOwner] = id;
            }

            ownerMaps[typecode].pop_back();
            --cnt;
            lightMap.erase(it);
            dirtyFlags[typecode] = true;
        }

        template <AllowedLightType T>
        void MarkDirty()
        {
            const int typecode = (int)T::type;
            dirtyFlags[typecode] = true;
        }

    private:
        std::vector<entt::entity> ownerMaps[(int)LightType::LIGHT_TYPE_CNT];
    };

    class LightManager
    {
    public:
        uint32_t lightCnts[(int)LightType::LIGHT_TYPE_CNT];

        LightManager()
        {
            init();
        }
        ~LightManager() {}
        LightManager(const LightManager &) = delete;
        LightManager &operator=(const LightManager &) = delete;

        void GetBindingInfos(std::vector<VkDescriptorSetLayoutBinding> &bindingInfos, DescriptorSetInfo &descriptorSetInfo);

        void GetDescriptorSetWrites(uint32_t id,
                                    std::vector<VkWriteDescriptorSet> &descriptorSetWrites,
                                    VkDescriptorSet descriptorSet,
                                    std::vector<VkDescriptorBufferInfo> &bufferInfos);

        void ConstructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);

        void Update(uint32_t currentFrame)
        {
            update(currentFrame);
        }

        void SetGlobalDescriptorSets(VkDescriptorSet *descriptorSet)
        {
            globalDescriptorSets = descriptorSet;
        }

        void BindSceneLighting(SceneLighting *lighting);

    private:
        uint32_t lightUpdateCnts[(int)LightType::LIGHT_TYPE_CNT];
        VkDescriptorSet *globalDescriptorSets;
        SceneLighting *sceneLighting;

        std::unique_ptr<DeviceBuffer> lightBuffers[(int)LightType::LIGHT_TYPE_CNT][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<DeviceBuffer> clusterBuffers[2][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<ComputePipeline> lightCullingTask;

        void init();
        void update(uint32_t currentFrame);
        void cullLights(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
    };
}

#endif
