#ifndef EDITOR_WIREFRAME_COLLISION_PASS_H
#define EDITOR_WIREFRAME_COLLISION_PASS_H

#include <render/subpass.hpp>
#include <render/hdr_color.hpp>
#include <render/buffer.hpp>
#include <render/pipeline.hpp>
#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

namespace vke_editor
{
    class WireframeCollisionPass : public vke_render::RenderPassBase
    {
    public:
        WireframeCollisionPass(vke_render::RenderContext *ctx, VkDescriptorSet *globalDescriptorSets);
        ~WireframeCollisionPass() override = default;

        void SetSelectedEntity(entt::entity entity) { selectedEntity = entity; }

        void Init(int subpassID,
                  vke_render::FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  vke_render::ResourceNodeIDMap &currentResourceNodeID) override;

        void Render(vke_render::TaskNode &node,
                    vke_render::FrameGraph &frameGraph,
                    VkCommandBuffer commandBuffer,
                    uint32_t currentFrame,
                    uint32_t imageIndex) override;

    private:
        struct PushConstants {
            glm::mat4 viewProj;
            glm::vec4 color;
        };

        entt::entity selectedEntity = entt::null;
        vke_render::HDRColorManager *hdrColorManager;

        std::shared_ptr<vke_render::ShaderModuleSet> shader;
        std::unique_ptr<vke_render::GraphicsPipeline> pipeline;

        std::unique_ptr<vke_render::HostCoherentBuffer> vertexBuffers[vke_render::MAX_FRAMES_IN_FLIGHT];

        vke_ds::id32_t taskNodeID;
        uint32_t hdrColorImageIndex;

        void constructFrameGraph(vke_render::FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 vke_render::ResourceNodeIDMap &currentResourceNodeID);
        void createGraphicsPipeline();
        void extractCollisionLines(entt::entity entity, std::vector<glm::vec3> &outVertices);
    };
}

#endif
