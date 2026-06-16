#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <render/buffer.hpp>
#include <render/descriptor.hpp>
#include <render/frame_graph.hpp>
#include <render/camera.hpp>
#include <render/render_config.hpp>
#include <asset.hpp>
#include <array>
#include <memory>

namespace vke_render
{
    struct CPULightData;

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

    class ShadowManager
    {
    public:
        ShadowManager(RenderContext *ctx, FrameGraph &frameGraph, std::shared_ptr<const CPULightData> cpuLightData, const CameraInfo *cameraInfo,
                      const DirectionalShadowConfig &config);
        ~ShadowManager();
        ShadowManager(const ShadowManager &) = delete;
        ShadowManager &operator=(const ShadowManager &) = delete;

        void UpdateShadow();
        void SyncToGPU(uint32_t currentFrame);
        void SetCPULightData(std::shared_ptr<const CPULightData> data) { cpuLightData = std::move(data); }

        VkImage *GetDirectionalShadowMapImages() { return shadowMapImages; }
        VkImageView GetDirectionalShadowCascadeView(uint32_t currentFrame, uint32_t cascade) const { return shadowCascadeImageViews[currentFrame][cascade]; }
        VkDescriptorSet GetShadowPassDescriptorSet(uint32_t currentFrame) const { return shadowPassDescriptorSets[currentFrame]; }
        VkDescriptorSet GetDeferredLightingDescriptorSet(uint32_t currentFrame) const { return deferredLightingDescriptorSets[currentFrame]; }
        const DirectionalShadowInfoCPU &GetDirectionalShadowInfo() const { return directionalShadowInfo; }
        const DirectionalShadowConfig &GetConfig() const { return config; }

    private:
        RenderContext *context;
        std::shared_ptr<const CPULightData> cpuLightData;
        const CameraInfo *cameraInfo;
        DirectionalShadowConfig config;
        std::unique_ptr<HostCoherentBuffer> shadowInfoBuffers[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet shadowPassDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet deferredLightingDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkImage shadowMapImages[MAX_FRAMES_IN_FLIGHT];
        VkImageView shadowMapImageViews[MAX_FRAMES_IN_FLIGHT];
        VkImageView shadowCascadeImageViews[MAX_FRAMES_IN_FLIGHT][MAX_DIRECTIONAL_SHADOW_CASCADE_CNT];
        VkSampler shadowMapSampler;
        DirectionalShadowInfoCPU directionalShadowInfo;

        void createDescriptorSets();
        void createImages();
        void createImageViews(uint32_t currentFrame);
        void createSampler();
        void updateShadowPassDescriptorSet(uint32_t currentFrame);
        void updateDeferredLightingDescriptorSet(uint32_t currentFrame);
        void onTransientResourcesReady(uint32_t currentFrame);
    };
}

#endif
