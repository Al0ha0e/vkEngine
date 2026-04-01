#ifndef UI_RENDER_H
#define UI_RENDER_H

#include <render/subpass.hpp>
#include <render/pipeline.hpp>
#include <render/buffer.hpp>
#include <asset.hpp>
#include <string_view>

namespace vke_render
{
    struct GlyphInstanceGPU
    {
        glm::vec4 quadRect;
        glm::vec4 uvRect;
        glm::vec4 color;
    };

    class UIRenderer : public RenderPassBase
    {
    public:
        static constexpr uint32_t MAX_UI_CHAR_CNT = 64;

        UIRenderer(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets)
            : RenderPassBase(UI_RENDERER, ctx, globalDescriptorSets) {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            font = vke_common::AssetManager::LoadFont(vke_common::BUILTIN_FONT_ARIAL_ID);
            textShader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_TEXT_ID);
            glyphInstanceBuffer = std::make_unique<HostCoherentBuffer>(sizeof(GlyphInstanceGPU) * MAX_UI_CHAR_CNT,
                                                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            SetText(100, 100, "Hello World");
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
            createDescriptorSet();
            createGraphicsPipeline();
        }

        void SetText(int x, int y, std::string_view s);
        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override;

    private:
        VkDescriptorSet uiDescriptorSet = VK_NULL_HANDLE;
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::shared_ptr<ShaderModuleSet> textShader;
        std::shared_ptr<vke_common::Font> font;
        std::unique_ptr<HostCoherentBuffer> glyphInstanceBuffer;
        uint32_t glyphInstanceCount = 0;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void createDescriptorSet();
        void createGraphicsPipeline();
    };
}

#endif
