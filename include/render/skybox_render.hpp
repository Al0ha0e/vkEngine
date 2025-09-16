#ifndef SKYBOX_RENDER_H
#define SKYBOX_RENDER_H

#include <render/renderinfo.hpp>
#include <render/subpass.hpp>
#include <nlohmann/json.hpp>

namespace vke_render
{
    struct AtmosphereParameter
    {
        glm::vec4 sunLightColor;
        float seaLevel;
        float planetRadius;
        float atmosphereHeight;
        float sunLightIntensity;
        float sunDiskAngle;
        float rayleighScatteringScale;
        float rayleighScatteringScalarHeight;
        float mieScatteringScale;
        float mieAnisotropy;
        float mieScatteringScalarHeight;
        float ozoneAbsorptionScale;
        float ozoneLevelCenterHeight;
        float ozoneLevelWidth;
        float aerialPerspectiveDistance;

        AtmosphereParameter() {}
        AtmosphereParameter(const nlohmann::json &json)
            : seaLevel(json["SeaLevel"]), planetRadius(json["PlanetRadius"]), atmosphereHeight(json["AtmosphereHeight"]),
              sunLightIntensity(json["SunLightIntensity"]), sunDiskAngle(json["SunDiskAngle"]),
              rayleighScatteringScale(json["RayleighScatteringScale"]), rayleighScatteringScalarHeight(json["RayleighScatteringScalarHeight"]),
              mieScatteringScale(json["MieScatteringScale"]), mieAnisotropy(json["MieAnisotropy"]), mieScatteringScalarHeight(json["MieScatteringScalarHeight"]),
              ozoneAbsorptionScale(json["OzoneAbsorptionScale"]), ozoneLevelCenterHeight(json["OzoneLevelCenterHeight"]), ozoneLevelWidth(json["OzoneLevelWidth"]),
              aerialPerspectiveDistance(json["AerialPerspectiveDistance"])
        {
            auto &sunColor = json["SunLightColor"];
            sunLightColor = glm::vec4(sunColor[0].get<float>(), sunColor[1].get<float>(), sunColor[2].get<float>(), 1.0);
        }
    };

    class SkyboxRenderer : public RenderPassBase
    {
    public:
        AtmosphereParameter atmosphereParameter;

        SkyboxRenderer(RenderContext *ctx, VkDescriptorSet globalDescriptorSet)
            : RenderPassBase(SKYBOX_RENDERER, ctx, globalDescriptorSet) {}

        ~SkyboxRenderer() {}

        void Init(int subpassID,
                  FrameGraph &frameGraph,
                  std::map<std::string, vke_ds::id32_t> &blackboard,
                  std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID) override
        {
            RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);
            initResources();
            createDescriptorSet();
            createGraphicsPipeline();
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
        }

        void Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex) override;

    private:
        VkDescriptorSet skyBoxDescriptorSet;
        std::unique_ptr<GraphicsPipeline> renderPipeline;
        std::unique_ptr<Mesh> skyboxMesh;
        std::unique_ptr<Material> skyboxMaterial;
        std::unique_ptr<ComputePipeline> skyLUTGenerationPipeline;
        std::unique_ptr<DeviceBuffer> atmosphereParamBuffer;
        std::unique_ptr<Texture2D> skyLUT;

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);
        void initResources();
        void createDescriptorSet();
        void createGraphicsPipeline();
        void generateLUT(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
    };
}

#endif