#ifndef SKYBOX_H
#define SKYBOX_H

#include <render/cubemap.hpp>
#include <render/frame_graph.hpp>
#include <render/light.hpp>
#include <render/renderinfo.hpp>
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

    class SkyboxManager
    {
    public:
        SkyboxManager(VkDescriptorSet *globalDescriptorSets, LightManager *lightManager)
            : globalDescriptorSets(globalDescriptorSets), lightManager(lightManager)
        {
            initResources();
            createDescriptorSet();
        }

        void ConstructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 CurrentResourceNodeIDMaps &currentResourceNodeID)
        {
            constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
        }

        CubeMap *GetSkyLUT(uint32_t currentFrame) { return skyLUTs[currentFrame].get(); }
        CubeMap *GetSkyIrradianceLUT(uint32_t currentFrame) { return skyIrradianceLUTs[currentFrame].get(); }
        CubeMap *GetSkySpecularLUT(uint32_t currentFrame) { return skySpecularLUTs[currentFrame].get(); }
        DeviceBuffer *GetAtmosphereParamBuffer() { return atmosphereParamBuffer.get(); }
        const glm::vec4 &GetSunLightDir()
        {
            updateSunLightDir();
            return sunLightDir;
        }

    private:
        static constexpr uint32_t SKY_IRRADIANCE_CUBE_SIZE = 64;
        static constexpr uint32_t SKY_VIEW_CUBE_SIZE = 256;
        static constexpr uint32_t SKY_SPECULAR_CUBE_SIZE = 128;
        static constexpr uint32_t SKY_SPECULAR_MIP_LEVELS = 6;

        VkDescriptorSet *globalDescriptorSets;
        LightManager *lightManager;
        glm::vec4 sunLightDir = glm::normalize(glm::vec4(1, 2, 0, 0));
        AtmosphereParameter atmosphereParameter;
        std::unique_ptr<ComputePipeline> skyLUTGenerationPipeline;
        std::unique_ptr<ComputePipeline> iblLUTGenerationPipeline;
        std::shared_ptr<Material> skyboxMaterial;
        std::unique_ptr<DeviceBuffer> atmosphereParamBuffer;
        std::unique_ptr<CubeMap> skyLUTs[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<CubeMap> skyIrradianceLUTs[MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<CubeMap> skySpecularLUTs[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet skyLUTDescriptorSets[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet skyIBLDescriptorSets[MAX_FRAMES_IN_FLIGHT];

        void constructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 CurrentResourceNodeIDMaps &currentResourceNodeID);
        void initResources();
        void createDescriptorSet();
        void updateSunLightDir();
        void generateLUT(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
        void generateIBLLUT(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
    };
}

#endif
