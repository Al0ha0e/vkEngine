#ifndef BLOOM_H
#define BLOOM_H

#include <render/pipeline.hpp>
#include <render/subpass.hpp>
#include <render/hdr_color.hpp>
#include <render/render_config.hpp>
#include <asset.hpp>
#include <functional>

namespace vke_render
{
    class BloomPass : public RenderPassBase
    {
    public:
        BloomPass(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets, HDRColorManager *hdrColorManager, const nlohmann::json &configJSON)
            : RenderPassBase(BLOOM_PASS, ctx, globalDescriptorSets),
              hdrColorManager(hdrColorManager),
              inputSampler(hdrColorManager->sampler),
              inputImageViewGetter([hdrColorManager](uint32_t currentFrame)
                                   { return hdrColorManager->GetImageView(currentFrame); }),
              sampler(VK_NULL_HANDLE),
              bloomHdrColorResourceID(0),
              constants(configJSON)
        {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                images[i] = VK_NULL_HANDLE;
                imageViews[i] = VK_NULL_HANDLE;
            }
            createImages();
            createSampler();
        }
        ~BloomPass() override;

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  ResourceNodeIDMap &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            bloomShader = vke_common::AssetManager::LoadVertFragShaderUnique(vke_common::BUILTIN_VFSHADER_BLOOM_ID);
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

            ImageResource *resource = static_cast<ImageResource *>(frameGraph.resources[bloomHdrColorResourceID].get());
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                resource->images[i] = images[i];
        }

        VkSampler GetOutputSampler() const { return sampler; }
        VkImageView GetOutputImageView(uint32_t currentFrame) const { return imageViews[currentFrame]; }
        void SetInput(VkSampler inputSampler, std::function<VkImageView(uint32_t)> imageViewGetter)
        {
            this->inputSampler = inputSampler;
            inputImageViewGetter = std::move(imageViewGetter);
        }

    private:
        VkDescriptorSet bloomDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::shared_ptr<ShaderModuleSet> bloomShader;
        HDRColorManager *hdrColorManager;
        VkSampler inputSampler;
        std::function<VkImageView(uint32_t)> inputImageViewGetter;
        VkSampler sampler;
        VkImage images[MAX_FRAMES_IN_FLIGHT];
        VkImageView imageViews[MAX_FRAMES_IN_FLIGHT];
        vke_ds::id32_t bloomHdrColorResourceID;
        vke_ds::id32_t bloomTaskNodeID;
        BloomConfig constants;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 ResourceNodeIDMap &currentResourceNodeID);
        void allocateDescriptorSet();
        void createGraphicsPipeline();
        void onTransientResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame);
        void createImages();
        void cleanupImages();
        void createImageView(uint32_t currentFrame);
        void cleanupImageViews();
        void createSampler();
    };
}

#endif
