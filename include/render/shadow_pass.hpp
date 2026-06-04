#ifndef SHADOW_PASS_H
#define SHADOW_PASS_H

#include <render/renderinfo.hpp>
#include <render/subpass.hpp>
#include <render/light.hpp>
#include <render/camera.hpp>
#include <asset.hpp>
#include <array>

namespace vke_render
{
    constexpr uint32_t DIRECTIONAL_SHADOW_MAP_SIZE = 4096;
    constexpr uint32_t DIRECTIONAL_SHADOW_CASCADE_CNT = 4;
    constexpr float DIRECTIONAL_SHADOW_MAX_DISTANCE = 80.0f;
    constexpr float DIRECTIONAL_SHADOW_SPLIT_LAMBDA = 0.75f;
    constexpr float DIRECTIONAL_SHADOW_DEPTH_MARGIN = 10.0f;
    constexpr uint32_t INVALID_SHADOW_LIGHT_INDEX = 0xFFFFFFFFu;

    struct DirectionalShadowInfoCPU
    {
        glm::uvec4 lightIndex;
        glm::mat4 lightViewProj[DIRECTIONAL_SHADOW_CASCADE_CNT];
        glm::vec4 cascadeSplits;
        glm::vec4 cascadeTexelSizes;

        DirectionalShadowInfoCPU()
            : lightIndex(INVALID_SHADOW_LIGHT_INDEX, 0, 0, 0), cascadeSplits(0.0f), cascadeTexelSizes(0.0f)
        {
            for (uint32_t i = 0; i < DIRECTIONAL_SHADOW_CASCADE_CNT; ++i)
                lightViewProj[i] = glm::mat4(1.0f);
        }
    };

    class ShadowPass : public RenderPassBase
    {
    public:
        ShadowPass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, LightManager *lightManager, const CameraInfo *cameraInfo)
            : RenderPassBase(SHADOW_PASS, ctx, globalDescriptorSets), lightManager(lightManager), cameraInfo(cameraInfo), shadowMapSampler(VK_NULL_HANDLE),
              shadowMapResourceID(0), shadowMapResourceNodeID(0), shadowTaskNodeID(0), unitAllocator(1)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                shadowMapImages[i] = VK_NULL_HANDLE;
                shadowMapImageViews[i] = VK_NULL_HANDLE;
                shadowDescriptorSets[i] = VK_NULL_HANDLE;
                for (uint32_t cascade = 0; cascade < DIRECTIONAL_SHADOW_CASCADE_CNT; ++cascade)
                    shadowCascadeImageViews[i][cascade] = VK_NULL_HANDLE;
            }
        }

        ~ShadowPass();

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  CurrentResourceNodeIDMaps &currentResourceNodeID) override;

        vke_ds::id64_t AddUnit(RenderUnit *unit, bool isSkin = false);
        void RemoveUnit(vke_ds::id64_t id);

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override;

        VkImageView GetDirectionalShadowMapView(uint32_t currentFrame) const { return shadowMapImageViews[currentFrame]; }
        VkSampler GetShadowMapSampler() const { return shadowMapSampler; }
        VkDescriptorBufferInfo GetDirectionalShadowBufferInfo(uint32_t currentFrame) { return shadowInfoBuffers[currentFrame].GetDescriptorBufferInfo(); }
        const DirectionalShadowInfoCPU &GetDirectionalShadowInfo() const { return directionalShadowInfo; }

    private:
        LightManager *lightManager;
        const CameraInfo *cameraInfo;
        std::shared_ptr<Material> shadowMaterial;
        std::shared_ptr<Material> shadowSkinMaterial;
        std::map<Material *, std::unique_ptr<RenderInfo>> renderInfoMap;
        std::map<vke_ds::id64_t, Material *> unitMaterialMap;
        std::vector<HostCoherentBuffer> shadowInfoBuffers;
        VkDescriptorSet shadowDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkImage shadowMapImages[MAX_FRAMES_IN_FLIGHT];
        VkImageView shadowMapImageViews[MAX_FRAMES_IN_FLIGHT];
        VkImageView shadowCascadeImageViews[MAX_FRAMES_IN_FLIGHT][DIRECTIONAL_SHADOW_CASCADE_CNT];
        VkSampler shadowMapSampler;
        DirectionalShadowInfoCPU directionalShadowInfo;
        vke_ds::id32_t shadowMapResourceID;
        vke_ds::id32_t shadowMapResourceNodeID;
        vke_ds::id32_t shadowTaskNodeID;
        vke_ds::NaiveIDAllocator<vke_ds::id64_t> unitAllocator;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 CurrentResourceNodeIDMaps &currentResourceNodeID);
        void createImages();
        void createImageViews(uint32_t currentFrame);
        void createSampler();
        void allocateDescriptorSets();
        void createGraphicsPipeline(RenderInfo &renderInfo, bool isSkin);
        void registerMaterial(std::shared_ptr<Material> &material, bool isSkin);
        void updateShadowInfo(uint32_t currentFrame);
        void onTransientResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame);
    };
}

#endif
