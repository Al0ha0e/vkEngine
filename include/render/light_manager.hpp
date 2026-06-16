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

        void Update(uint32_t currentFrame, bool cameraUpdated)
        {
            update(currentFrame, cameraUpdated);
        }

        void SetGlobalDescriptorSets(VkDescriptorSet *descriptorSet)
        {
            globalDescriptorSets = descriptorSet;
        }

        DirectionalLight *GetSun();
        uint32_t GetLightCnt(LightType type) const { return cpuLightData->lightCnts[(int)type]; }
        const std::shared_ptr<CPULightData> &GetCPULightData() const { return cpuLightData; }
        void LoadSceneLightData(std::shared_ptr<CPULightData> lighting);
        std::shared_ptr<CPULightData> ToSceneLightData() const;
        void ClearLights();

        template <AllowedLightType T, typename... Args>
        void AddLight(entt::entity entity, Args &&...args)
        {
            const int typecode = (int)T::type;
            uint32_t &cnt = cpuLightData->lightCnts[typecode];
            VKE_FATAL_IF(cnt >= MAX_LIGHT_CNTS[typecode], "NO MORE LIGHT OF TYPE {}", typecode)

            const vke_ds::id32_t id = cnt++;
            ownerMaps[typecode].push_back(entity);
            std::construct_at(reinterpret_cast<T *>(cpuLightData->cpuLightBuffers[typecode]->data) + id, std::forward<Args>(args)...);

            cpuLightData->entityToLight[typecode][entity] = id;
            dirtyFlags[typecode] = true;
        }

        template <AllowedLightType T>
        bool HasLight(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            return cpuLightData->entityToLight[typecode].find(entity) != cpuLightData->entityToLight[typecode].end();
        }

        template <AllowedLightType T>
        T &GetLightWithoutCheck(entt::entity entity)
        {
            const int typecode = (int)T::type;
            vke_ds::id32_t id = cpuLightData->entityToLight[typecode].find(entity)->second;
            return reinterpret_cast<T *>(cpuLightData->cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        const T &GetLightWithoutCheck(entt::entity entity) const
        {
            const int typecode = (int)T::type;
            vke_ds::id32_t id = cpuLightData->entityToLight[typecode].find(entity)->second;
            return reinterpret_cast<const T *>(cpuLightData->cpuLightBuffers[typecode]->data)[id];
        }

        template <AllowedLightType T>
        void RemoveLight(entt::entity entity)
        {
            const int typecode = (int)T::type;
            auto &lightMap = cpuLightData->entityToLight[typecode];
            auto it = lightMap.find(entity);
            if (it == lightMap.end())
                return;

            vke_ds::id32_t id = it->second;
            uint32_t &cnt = cpuLightData->lightCnts[typecode];
            VKE_FATAL_IF(id >= cnt, "LIGHT NOT EXIST")

            const uint32_t last = cnt - 1;
            if (id != last)
            {
                const size_t size = LIGHT_SIZES[typecode];
                char *base = reinterpret_cast<char *>(cpuLightData->cpuLightBuffers[typecode]->data);
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

    private:
        bool dirtyFlags[(int)LightType::LIGHT_TYPE_CNT];
        uint32_t lightUpdateCnts[(int)LightType::LIGHT_TYPE_CNT];
        VkDescriptorSet *globalDescriptorSets;
        std::vector<entt::entity> ownerMaps[(int)LightType::LIGHT_TYPE_CNT];
        std::shared_ptr<CPULightData> cpuLightData;

        std::unique_ptr<DeviceBuffer> lightBuffers[(int)LightType::LIGHT_TYPE_CNT][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<DeviceBuffer> clusterBuffers[2][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<ComputePipeline> lightCullingTask;
        std::unique_ptr<ShadowManager> shadowManager;

        void init();
        void update(uint32_t currentFrame, bool cameraUpdated);
        void cullLights(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
    };
}

#endif
