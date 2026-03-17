#ifndef LIGHT_H
#define LIGHT_H

#include <render/buffer.hpp>
#include <render/descriptor.hpp>
#include <render/pipeline.hpp>
#include <render/frame_graph.hpp>
#include <concepts>
#include <type_traits>
#include <memory>

namespace vke_render
{
    enum class LightType
    {
        DIRECTIONAL_LIGHT,
        POINT_LIGHT,
        SPOT_LIGHT,
        LIGHT_TYPE_CNT
    };

    struct DirectionalLight
    {
        static constexpr LightType type = LightType::DIRECTIONAL_LIGHT;
        glm::vec4 direction;          // xyz
        glm::vec4 colorWithIntensity; // w: intensity

        DirectionalLight() : direction(1, 0, 0, 0), colorWithIntensity(0) {}
        DirectionalLight(glm::vec4 direction, glm::vec4 color) : direction(direction), colorWithIntensity(color) {}
    };

    struct PointLight
    {
        static constexpr LightType type = LightType::POINT_LIGHT;
        glm::vec4 positionWithRadius; // w: radius
        glm::vec4 colorWithIntensity; // w: intensity

        PointLight() : positionWithRadius(0), colorWithIntensity(0) {}
        PointLight(glm::vec4 positionWithRadius, glm::vec4 colorWithIntensity)
            : positionWithRadius(positionWithRadius), colorWithIntensity(colorWithIntensity) {}
    };

    struct SpotLight
    {
        static constexpr LightType type = LightType::SPOT_LIGHT;
        glm::vec4 positionWithRadius; // w: radius
        glm::vec4 direction;          // xyz
        glm::vec4 colorWithIntensity; // w: intensity
        glm::vec4 cone;               // x:inner, y: outer

        SpotLight() : positionWithRadius(0), direction(1, 0, 0, 0), colorWithIntensity(0), cone(0) {}
        SpotLight(glm::vec4 positionWithRadius, glm::vec4 direction, glm::vec4 colorWithIntensity, glm::vec4 cone)
            : positionWithRadius(positionWithRadius), direction(direction), colorWithIntensity(colorWithIntensity), cone(cone) {}
    };

    template <typename T>
    concept AllowedLightType = std::same_as<T, DirectionalLight> || std::same_as<T, PointLight> || std::same_as<T, SpotLight>;

    const uint32_t MAX_DIRECTIONAL_LIGHT_CNT = 4;
    const uint32_t MAX_POINT_LIGHT_CNT = 1024;
    const uint32_t MAX_SPOT_LIGHT_CNT = 512;
    const uint32_t MAX_LIGHT_PER_CLUSTER = 64;
    const uint32_t CLUSTER_DIM_X = 16;
    const uint32_t CLUSTER_DIM_Y = 8;
    const uint32_t CLUSTER_DIM_Z = 16;

    constexpr uint32_t MAX_LIGHT_CNTS[] = {MAX_DIRECTIONAL_LIGHT_CNT, MAX_POINT_LIGHT_CNT, MAX_SPOT_LIGHT_CNT};
    constexpr size_t LIGHT_SIZES[] = {sizeof(DirectionalLight), sizeof(PointLight), sizeof(SpotLight)};
    constexpr uint32_t LIGHT_MAP_ST[] = {0, MAX_DIRECTIONAL_LIGHT_CNT, MAX_DIRECTIONAL_LIGHT_CNT + MAX_POINT_LIGHT_CNT,
                                         MAX_DIRECTIONAL_LIGHT_CNT + MAX_POINT_LIGHT_CNT + MAX_SPOT_LIGHT_CNT};

    class LightManager
    {
    public:
        uint32_t lightCnts[(int)LightType::LIGHT_TYPE_CNT];

        LightManager()
        {
            init();
        }
        ~LightManager() {}
        LightManager(const LightManager &) = delete;
        LightManager &operator=(const LightManager &) = delete;

        void GetBindingInfos(std::vector<VkDescriptorSetLayoutBinding> &bindingInfos, DescriptorSetInfo &descriptorSetInfo);

        void GetDescriptorSetWrites(uint32_t id,
                                    std::vector<VkWriteDescriptorSet> &descriptorSetWrites,
                                    VkDescriptorSet descriptorSet,
                                    std::vector<VkDescriptorBufferInfo> &bufferInfos);

        void ConstructFrameGraph(FrameGraph &frameGraph,
                                 std::map<std::string, vke_ds::id32_t> &blackboard,
                                 std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID);

        void Update(uint32_t currentFrame)
        {
            update(currentFrame);
        }

        template <AllowedLightType T, typename... Args>
        vke_ds::id32_t AddLight(Args &&...args)
        {
            const int typecode = (int)T::type;
            vke_ds::id32_t ret = lightCnts[typecode];
            if (ret >= MAX_LIGHT_CNTS[typecode])
                return MAX_LIGHT_CNTS[typecode];

            lightMap[LIGHT_MAP_ST[typecode] + ret] = ret;
            ++lightCnts[typecode];

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                std::construct_at(
                    reinterpret_cast<T *>(lightBuffers[typecode][i]->data) + ret,
                    std::forward<Args>(args)...);

            MarkDirty<T>();
            return ret;
        }

        template <vke_render::AllowedLightType T>
        T &GetLight(vke_ds::id32_t id)
        {
            const int typecode = (int)T::type;
            VKE_FATAL_IF(id >= lightCnts[typecode], "LIGHT NOT EXIST")
            return ((T *)lightBuffers[typecode][0]->data)[lightMap[LIGHT_MAP_ST[typecode] + id]];
        }

        void RemoveLight(LightType type, vke_ds::id32_t id)
        {
            const int typecode = (int)type;
            uint32_t &cnt = lightCnts[typecode];
            if (id >= cnt)
                return;

            uint32_t last = --cnt;
            if (id != last)
            {
                size_t size = LIGHT_SIZES[typecode];
                for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
                {
                    char *base = (char *)lightBuffers[typecode][i]->data;
                    std::memcpy(
                        base + id * size,
                        base + last * size,
                        size);
                }
                lightMap[LIGHT_MAP_ST[typecode] + id] = lightMap[LIGHT_MAP_ST[typecode] + last];
            }
            MarkDirty(type);
        }

        template <vke_render::AllowedLightType T>
        void MarkDirty()
        {
            const int typecode = (int)T::type;
            lightUpdateCnts[typecode] = MAX_FRAMES_IN_FLIGHT;
        }

        void MarkDirty(const LightType type)
        {
            lightUpdateCnts[(int)type] = MAX_FRAMES_IN_FLIGHT;
        }

        void SetGlobalDescriptorSets(VkDescriptorSet *descriptorSet)
        {
            globalDescriptorSets = descriptorSet;
        }

    private:
        uint32_t lightUpdateCnts[(int)LightType::LIGHT_TYPE_CNT];
        uint32_t lightMap[LIGHT_MAP_ST[(int)LightType::LIGHT_TYPE_CNT]];
        VkDescriptorSet *globalDescriptorSets;

        std::unique_ptr<StagedBuffer> lightBuffers[(int)LightType::LIGHT_TYPE_CNT][MAX_FRAMES_IN_FLIGHT];
        std::unique_ptr<DeviceBuffer> clusterBuffers[2];
        std::unique_ptr<ComputePipeline> lightCullingTask;

        void init();
        void update(uint32_t currentFrame);
        void cullLights(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
    };
}

#endif