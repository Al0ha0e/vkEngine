#ifndef SSAO_H
#define SSAO_H

#include <render/pipeline.hpp>
#include <render/subpass.hpp>
#include <render/gbuffer.hpp>
#include <render/render_config.hpp>
#include <asset.hpp>

namespace vke_render
{
    class SSAOPass : public RenderPassBase
    {
    public:
        static constexpr VkFormat SSAO_FORMAT = VK_FORMAT_R8_UNORM;

        SSAOPass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, const nlohmann::json &configJSON)
            : RenderPassBase(SSAO_PASS, ctx, globalDescriptorSets),
              gbuffer(nullptr),
              sampler(VK_NULL_HANDLE),
              ssaoResourceID(0),
              ssaoRawResourceID(0),
              ssaoTaskNodeID(0),
              ssaoBlurTaskNodeID(0),
              constants(configJSON)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                images[i] = VK_NULL_HANDLE;
                imageViews[i] = VK_NULL_HANDLE;
                rawImages[i] = VK_NULL_HANDLE;
                rawImageViews[i] = VK_NULL_HANDLE;
            }
            createImages();
            createSampler();
        }

        ~SSAOPass() override;

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  CurrentResourceNodeIDMaps &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            gbuffer = GBuffer::GetInstance();
            ssaoShader = vke_common::AssetManager::LoadVertFragShaderUnique(vke_common::BUILTIN_VFSHADER_SSAO_ID);
            ssaoBlurShader = vke_common::AssetManager::LoadVertFragShaderUnique(vke_common::BUILTIN_VFSHADER_SSAO_BLUR_ID);
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            allocateDescriptorSet();
            createGraphicsPipeline();
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override
        {
            context = ctx;
            cleanupImageViews();
            cleanupImages();
            createImages();

            ImageResource *resource = static_cast<ImageResource *>(frameGraph.transientResources[ssaoResourceID].get());
            ImageResource *rawResource = static_cast<ImageResource *>(frameGraph.transientResources[ssaoRawResourceID].get());
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                resource->images[i] = images[i];
                rawResource->images[i] = rawImages[i];
            }
        }

        VkSampler GetOutputSampler() const { return sampler; }
        VkImageView GetOutputImageView(uint32_t currentFrame) const { return imageViews[currentFrame]; }
        vke_ds::id32_t GetResourceID() const { return ssaoResourceID; }

    private:
        VkDescriptorSet ssaoDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet ssaoBlurDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::unique_ptr<GraphicsPipeline> blurPipeline;
        std::shared_ptr<ShaderModuleSet> ssaoShader;
        std::shared_ptr<ShaderModuleSet> ssaoBlurShader;
        GBuffer *gbuffer;
        VkSampler sampler;
        VkImage images[MAX_FRAMES_IN_FLIGHT];
        VkImageView imageViews[MAX_FRAMES_IN_FLIGHT];
        VkImage rawImages[MAX_FRAMES_IN_FLIGHT];
        VkImageView rawImageViews[MAX_FRAMES_IN_FLIGHT];
        vke_ds::id32_t ssaoResourceID;
        vke_ds::id32_t ssaoRawResourceID;
        vke_ds::id32_t ssaoTaskNodeID;
        vke_ds::id32_t ssaoBlurTaskNodeID;
        SSAOConfig constants;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 CurrentResourceNodeIDMaps &currentResourceNodeID);
        void allocateDescriptorSet();
        void createGraphicsPipeline();
        void onSSAORawResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame);
        void onSSAOBlurResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame);
        void renderFullscreen(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkImageView outputImageView,
                              GraphicsPipeline *pipeline, VkDescriptorSet descriptorSet, const float clearValue);
        void createImages();
        void cleanupImages();
        void createImageView(uint32_t currentFrame);
        void cleanupImageViews();
        void createSampler();
    };
}

#endif
