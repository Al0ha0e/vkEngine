#ifndef BASE_RENDER_H
#define BASE_RENDER_H

#include <render/environment.hpp>
#include <render/renderinfo.hpp>
#include <render/material.hpp>
#include <render/mesh.hpp>
#include <render/subpass.hpp>

namespace vke_render
{
    class BaseRenderer : public RenderPassBase
    {
    public:
        std::vector<DescriptorInfo> globalDescriptorInfos;
        DescriptorSetInfo globalDescriptorSetInfo;
        VkDescriptorSet globalDescriptorSet;

        BaseRenderer(RenderContext *ctx, VkBuffer camBuffer)
            : globalDescriptorSetInfo(nullptr, 0, 0, 0, 0), RenderPassBase(BASE_RENDERER, ctx, camBuffer) {}

        ~BaseRenderer() {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            environment = RenderEnvironment::GetInstance();
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            createGlobalDescriptorSet();
            registerCamera();
            createSkyBox();
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        RenderEnvironment *environment;
        std::unique_ptr<RenderInfo> renderInfo;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createGlobalDescriptorSet();
        void createSkyBox();
        void createGraphicsPipeline();

        void registerCamera()
        {
            DescriptorInfo &info = globalDescriptorInfos[0];
            VkDescriptorBufferInfo bufferInfo{};
            InitDescriptorBufferInfo(bufferInfo, camInfoBuffer, 0, info.bufferSize);
            VkWriteDescriptorSet descriptorWrite{};
            ConstructDescriptorSetWrite(descriptorWrite, globalDescriptorSet, info, &bufferInfo);
            vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, 1, &descriptorWrite, 0, nullptr);
        }
    };
}

#endif