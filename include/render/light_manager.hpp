#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <render/light.hpp>
#include <render/descriptor.hpp>
#include <render/pipeline.hpp>
#include <render/frame_graph.hpp>
#include <render/shadow_manager.hpp>
#include <render/camera.hpp>
#include <render/render_config.hpp>
#include <cstring>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace vke_render
{
    class LightManager
    {
    public:
        LightManager(RenderContext *ctx, FrameGraph &frameGraph, const CameraInfo *cameraInfo, const DirectionalShadowConfig &config);
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
                                 ResourceNodeIDMap &currentResourceNodeID);

        void Update(uint32_t currentFrame, bool cameraUpdated);

        void SetGlobalDescriptorSets(VkDescriptorSet *descriptorSet)
        {
            globalDescriptorSets = descriptorSet;
        }

        DirectionalLight *GetSun();
        uint32_t GetLightCnt(LightType type) const { return lightCnts[(int)type]; }
        void ClearLights();

        template <AllowedLightType T, typename... Args>
        void AddLight(entt::entity entity, Args &&...args)
        {
            const int typecode = (int)T::type;
            uint32_t &cnt = lightCnts[typecode];
            VKE_FATAL_IF(cnt >= MAX_LIGHT_CNTS[typecode], "NO MORE LIGHT OF TYPE {}", typecode)

            const vke_ds::id32_t id = cnt++;
            ownerMaps[typecode].push_back(entity);
            std::construct_at(reinterpret_cast<T *>(cpuLightBuffers[typecode]->data) + id, std::forward<Args>(args)...);

            entityToLight[typecode][entity] = id;
            if constexpr (std::same_as<T, SpotLight>)
            {
                SpotLight &light = GetLightWithoutCheckByID<SpotLight>(id);
                if (light.CastShadow())
                    shadowManager->ActivateSpotShadow(entity, light);
            }
            dirtyFlags[typecode] = true;
        }

        template <AllowedLightType T>
        void AddLight(entt::entity entity, const T &light)
        {
            const int typecode = (int)T::type;
            uint32_t &cnt = lightCnts[typecode];
            VKE_FATAL_IF(cnt >= MAX_LIGHT_CNTS[typecode], "NO MORE LIGHT OF TYPE {}", typecode)

            const vke_ds::id32_t id = cnt++;
            ownerMaps[typecode].push_back(entity);
            reinterpret_cast<T *>(cpuLightBuffers[typecode]->data)[id] = light;

            entityToLight[typecode][entity] = id;
            if constexpr (std::same_as<T, SpotLight>)
            {
                SpotLight &light = GetLightWithoutCheckByID<SpotLight>(id);
                if (light.CastShadow())
                    shadowManager->ActivateSpotShadow(entity, light);
            }
            dirtyFlags[typecode] = true;
        }

        template <AllowedLightType T>
        bool HasLight(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            return entityToLight[typecode].find(entity) != entityToLight[typecode].end();
        }

        template <AllowedLightType T>
        T &GetLightWithoutCheckByEntity(entt::entity entity)
        {
            const int typecode = (int)T::type;
            vke_ds::id32_t id = entityToLight[typecode].find(entity)->second;
            return reinterpret_cast<T *>(cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        const T &GetLightWithoutCheckByEntity(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            vke_ds::id32_t id = entityToLight[typecode].find(entity)->second;
            return reinterpret_cast<const T *>(cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        T &GetLightWithoutCheckByID(vke_ds::id32_t id)
        {
            const int typecode = (int)T::type;
            return reinterpret_cast<T *>(cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        const T &GetLightWithoutCheckByID(vke_ds::id32_t id) const
        {
            const int typecode = (int)T::type;
            return reinterpret_cast<T *>(cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        void RemoveLight(entt::entity entity)
        {
            const int typecode = (int)T::type;
            auto &lightMap = entityToLight[typecode];
            auto it = lightMap.find(entity);
            if (it == lightMap.end())
                return;
            vke_ds::id32_t id = it->second;

            if constexpr (std::same_as<T, SpotLight>)
            {
                SpotLight &light = GetLightWithoutCheckByID<SpotLight>(id);
                shadowManager->DeactivateSpotShadow(entity, light);
            }

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

        ShadowManager *GetShadowManager() { return shadowManager.get(); }
        const ShadowManager *GetShadowManager() const { return shadowManager.get(); }
        void UpdateSpotShadow(entt::entity entity);
        uint32_t ActivateSpotShadow(entt::entity entity);
        void DeactivateSpotShadow(entt::entity entity);

    private:
        std::unique_ptr<HostCoherentBuffer> cpuLightBuffers[(int)LightType::LIGHT_TYPE_CNT];
        uint32_t lightCnts[(int)LightType::LIGHT_TYPE_CNT];
        std::unordered_map<entt::entity, vke_ds::id32_t> entityToLight[(int)LightType::LIGHT_TYPE_CNT];
        std::vector<entt::entity> ownerMaps[(int)LightType::LIGHT_TYPE_CNT];
        bool dirtyFlags[(int)LightType::LIGHT_TYPE_CNT];
        uint32_t lightUpdateCnts[(int)LightType::LIGHT_TYPE_CNT];
        VkDescriptorSet *globalDescriptorSets;

        std::unique_ptr<DeviceBuffer> lightBuffers[(int)LightType::LIGHT_TYPE_CNT][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<DeviceBuffer> clusterBuffers[2][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<ComputePipeline> lightCullingTask;
        std::unique_ptr<ShadowManager> shadowManager;

        void init();
        void cullLights(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
    };
}

#endif
