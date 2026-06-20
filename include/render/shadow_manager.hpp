#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <render/buffer.hpp>
#include <render/descriptor.hpp>
#include <render/frame_graph.hpp>
#include <render/camera.hpp>
#include <render/render_config.hpp>
#include <asset.hpp>
#include <array>
#include <entt/entity/entity.hpp>
#include <memory>
#include <vector>

namespace vke_render
{
    struct CPULightData;
    struct SpotLight;

    constexpr uint32_t INVALID_SHADOW_LIGHT_INDEX = 0xFFFFFFFFu;

    struct DirectionalShadowInfoCPU
    {
        glm::uvec4 lightIndex;
        glm::uvec4 cascadeCnt;
        glm::mat4 lightViewProj[MAX_DIRECTIONAL_SHADOW_CASCADE_CNT];
        glm::vec4 cascadeSplits;
        glm::vec4 cascadeTexelSizes;

        DirectionalShadowInfoCPU()
            : lightIndex(INVALID_SHADOW_LIGHT_INDEX, 0, 0, 0), cascadeCnt(0), cascadeSplits(0.0f), cascadeTexelSizes(0.0f)
        {
            for (uint32_t i = 0; i < MAX_DIRECTIONAL_SHADOW_CASCADE_CNT; ++i)
                lightViewProj[i] = glm::mat4(1.0f);
        }
    };

    struct SpotShadowInfoCPU
    {
        glm::mat4 lightViewProj;

        SpotShadowInfoCPU() : lightViewProj(1.0f) {}
    };

    class ShadowManager
    {
    public:
        ShadowManager(RenderContext *ctx, FrameGraph &frameGraph, std::shared_ptr<CPULightData> cpuLightData, const CameraInfo *cameraInfo,
                      const DirectionalShadowConfig &directionalConfig);
        ~ShadowManager();
        ShadowManager(const ShadowManager &) = delete;
        ShadowManager &operator=(const ShadowManager &) = delete;

        void UpdateDirectionalShadowInfo();
        void CalcSpotShadowVPMatrix(SpotLight &light);
        void SyncDirectionalShadowToGPU(uint32_t currentFrame);
        void SyncSpotShadowToGPU(uint32_t currentFrame);
        void SetCPULightData(std::shared_ptr<CPULightData> data);
        uint32_t ActivateSpotShadow(entt::entity lightEntity, SpotLight &light);
        void DeactivateSpotShadow(entt::entity lightEntity, SpotLight &light);

        VkImage *GetDirectionalShadowMapImages() { return directionalShadowMapImages; }
        VkImage *GetSpotShadowMapImages() { return spotShadowMapImages; }
        VkImageView GetDirectionalShadowCascadeView(uint32_t currentFrame, uint32_t cascade) const { return directionalShadowCascadeImageViews[currentFrame][cascade]; }
        VkImageView GetSpotShadowMapLayerView(uint32_t currentFrame, uint32_t slot) const { return spotShadowMapLayerViews[currentFrame][slot]; }
        VkDescriptorSet GetShadowPassDescriptorSet(uint32_t currentFrame) const { return shadowPassDescriptorSets[currentFrame]; }
        VkDescriptorSet GetDeferredLightingDescriptorSet(uint32_t currentFrame) const { return deferredLightingDescriptorSets[currentFrame]; }
        const DirectionalShadowInfoCPU &GetDirectionalShadowInfo() const { return directionalShadowInfo; }
        bool IsSpotShadowSlotActive(uint32_t slot) const { return spotShadowLightEntities[slot] != entt::null; }
        const DirectionalShadowConfig &GetDirectionalConfig() const { return directionalConfig; }
        const SpotShadowConfig &GetSpotConfig() const { return spotConfig; }

    private:
        RenderContext *context;
        std::shared_ptr<CPULightData> cpuLightData;
        const CameraInfo *cameraInfo;
        DirectionalShadowConfig directionalConfig;
        SpotShadowConfig spotConfig;
        std::unique_ptr<HostCoherentBuffer> directionalShadowInfoBuffers[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<HostCoherentBuffer> spotShadowInfoBuffers[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet shadowPassDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet deferredLightingDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkImage directionalShadowMapImages[MAX_FRAMES_IN_FLIGHT];
        VkImageView directionalShadowMapImageViews[MAX_FRAMES_IN_FLIGHT];
        VkImageView directionalShadowCascadeImageViews[MAX_FRAMES_IN_FLIGHT][MAX_DIRECTIONAL_SHADOW_CASCADE_CNT];
        VkImage spotShadowMapImages[MAX_FRAMES_IN_FLIGHT];
        VkImageView spotShadowMapImageViews[MAX_FRAMES_IN_FLIGHT];
        VkImageView spotShadowMapLayerViews[MAX_FRAMES_IN_FLIGHT][MAX_SPOT_LIGHT_SHADOW_CNT];
        VkSampler shadowMapSampler;
        DirectionalShadowInfoCPU directionalShadowInfo;
        SpotShadowInfoCPU spotShadowInfos[MAX_SPOT_LIGHT_SHADOW_CNT];
        entt::entity spotShadowLightEntities[MAX_SPOT_LIGHT_SHADOW_CNT];
        uint32_t spotShadowUpdateCnts[MAX_SPOT_LIGHT_SHADOW_CNT];

        void createDescriptorSets();
        void createDirectionalImages();
        void createSpotShadowImages();
        void createDirectionalImageViews(uint32_t currentFrame);
        void createSpotShadowImageViews(uint32_t currentFrame);
        void createSampler();
        void updateDeferredLightingDescriptorSet(uint32_t currentFrame);
        void onTransientResourcesReady(uint32_t currentFrame);
        int32_t findSpotShadowSlot(entt::entity lightEntity) const;
        void clearLights();
    };
}

#endif
