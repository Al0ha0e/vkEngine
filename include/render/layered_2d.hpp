#ifndef LAYERED_2D_RENDER_H
#define LAYERED_2D_RENDER_H

#include <asset.hpp>
#include <render/glyph_manager.hpp>
#include <render/material.hpp>
#include <render/pipeline.hpp>
#include <render/subpass.hpp>
#include <limits>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke_render
{
    struct Layered2DRenderPushConstants
    {
        glm::mat4 model{1.0f};
        glm::vec2 viewportSize{1.0f};
        glm::vec2 atlasSize{1.0f};
    };

    class Layered2DRenderUnit
    {
    public:
        Layered2DRenderUnit(vke_ds::id32_t id, const glm::mat4 *modelMatrix)
            : id(id), modelMatrix(modelMatrix) {}

        const vke_ds::id32_t id;
        const glm::mat4 *modelMatrix;
        std::vector<GlyphID> glyphIDs;

        void SetGlyphIDs(std::vector<GlyphID> ids);
        void Render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                    const glm::vec2 &viewportSize, const glm::vec2 &atlasSize) const;

    private:
        std::unique_ptr<HostCoherentBuffer> glyphIDBuffer;
    };

    class Layered2DRenderInfo
    {
    public:
        Layered2DRenderInfo(std::shared_ptr<Material> material, RenderContext *context);

        std::shared_ptr<Material> material;
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        VkDescriptorSet commonDescriptorSet = VK_NULL_HANDLE;
        std::map<vke_ds::id32_t, Layered2DRenderUnit *> units;

        void AddUnit(Layered2DRenderUnit *unit) { units[unit->id] = unit; }
        void RemoveUnit(vke_ds::id32_t unitID) { units.erase(unitID); }
        bool Empty() const { return units.empty(); }
        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet rendererDescriptorSet,
                    const glm::vec2 &viewportSize, const glm::vec2 &atlasSize) const;

    private:
        void createPipeline(RenderContext *context);
    };

    class Layered2DRenderLayer
    {
    public:
        explicit Layered2DRenderLayer(vke_ds::id32_t id) : id(id) {}

        const vke_ds::id32_t id;

        void AddUnit(const std::shared_ptr<Material> &material, Layered2DRenderUnit *unit, RenderContext *context);
        void RemoveUnit(Material *material, vke_ds::id32_t unitID);
        bool Empty() const { return renderInfos.empty(); }
        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet rendererDescriptorSet,
                    const glm::vec2 &viewportSize, const glm::vec2 &atlasSize) const;

    private:
        std::map<Material *, std::unique_ptr<Layered2DRenderInfo>> renderInfos;
    };

    class Layered2DRenderer : public RenderPassBase
    {
    public:
        Layered2DRenderer(RenderContext *ctx, VkDescriptorSet *globalDescriptorSets,
                          GlyphManager *glyphManager)
            : RenderPassBase(LAYERED_2D_RENDERER, ctx, globalDescriptorSets),
              glyphManager(glyphManager) {}

        void Init(int subpassID, FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  ResourceNodeIDMap &currentResourceNodeID) override;
        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer,
                    uint32_t currentFrame, uint32_t imageIndex) override;
        void OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) override;

        bool AllocateUnit(vke_ds::id32_t unitID, const std::shared_ptr<Material> &material,
                          const glm::mat4 *modelMatrix, const std::vector<GlyphID> &glyphIDs);
        void DestroyUnit(vke_ds::id32_t unitID);
        bool UpdateUnitGlyphIDs(vke_ds::id32_t unitID, const std::vector<GlyphID> &glyphIDs);
        bool AddUnitToLayer(vke_ds::id32_t unitID, vke_ds::id32_t layerID);
        void RemoveUnitFromLayer(vke_ds::id32_t unitID, vke_ds::id32_t layerID);
        void SetLayerOrder(const std::vector<vke_ds::id32_t> &backToFrontLayerIDs);

        Layered2DRenderUnit *GetUnit(vke_ds::id32_t unitID);
        std::shared_ptr<Material> GetDefaultMaterial() const { return defaultMaterial; }
        std::shared_ptr<vke_common::Font> GetFont() const { return font; }

    private:
        struct UnitState
        {
            std::unique_ptr<Layered2DRenderUnit> unit;
            std::shared_ptr<Material> material;
            vke_ds::id32_t layerID = std::numeric_limits<vke_ds::id32_t>::max();
        };

        GlyphManager *glyphManager;
        VkDescriptorSet rendererDescriptorSets[MAX_FRAMES_IN_FLIGHT]{};
        std::shared_ptr<ShaderModuleSet> textShader;
        std::shared_ptr<Material> defaultMaterial;
        std::shared_ptr<vke_common::Font> font;
        std::unique_ptr<HostCoherentBuffer> dynamicAtlasStagingBuffers[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<Texture2D> dynamicAtlasTextures[MAX_FRAMES_IN_FLIGHT];
        std::map<vke_ds::id32_t, UnitState> units;
        std::map<vke_ds::id32_t, std::unique_ptr<Layered2DRenderLayer>> layers;
        std::vector<vke_ds::id32_t> layerOrder;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 ResourceNodeIDMap &currentResourceNodeID);
        void createDescriptorSets();
        void syncDynamicAtlas(VkCommandBuffer commandBuffer, uint32_t currentFrame);
    };
}

#endif
