#include <render/light.hpp>
#include <asset.hpp>

namespace vke_render
{
    void LightManager::init()
    {
        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            lightCnts[i] = 0;
            lightUpdateCnts[i] = 0;
            dirtyFlags[i] = false;
            cpuLightBuffers[i] = std::make_unique<HostCoherentBuffer>(LIGHT_SIZES[i] * MAX_LIGHT_CNTS[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        }

        auto lightCullingShader = vke_common::AssetManager::LoadComputeShader(vke_common::BUILTIN_COMPUTE_SHADER_LIGHTCULL_ID);
        lightCullingTask = std::make_unique<ComputePipeline>(lightCullingShader);

        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
            for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                lightBuffers[i][j] = std::make_unique<DeviceBuffer>(LIGHT_SIZES[i] * MAX_LIGHT_CNTS[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j)
                clusterBuffers[i][j] = std::make_unique<DeviceBuffer>(CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z * MAX_LIGHT_PER_CLUSTER * sizeof(uint32_t),
                                                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    void LightManager::GetBindingInfos(std::vector<VkDescriptorSetLayoutBinding> &bindingInfos, DescriptorSetInfo &descriptorSetInfo)
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

        for (int i = 1; i <= (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            layoutBinding.binding = i;
            bindingInfos.push_back(layoutBinding);
        }

        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        for (int i = (int)LightType::LIGHT_TYPE_CNT + 1; i <= (int)LightType::LIGHT_TYPE_CNT + 2; ++i)
        {
            layoutBinding.binding = i;
            bindingInfos.push_back(layoutBinding);
        }

        descriptorSetInfo.AddCnt(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (int)LightType::LIGHT_TYPE_CNT);
        descriptorSetInfo.AddCnt(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2);
    }

    void LightManager::GetDescriptorSetWrites(uint32_t id,
                                              std::vector<VkWriteDescriptorSet> &descriptorSetWrites,
                                              VkDescriptorSet descriptorSet,
                                              std::vector<VkDescriptorBufferInfo> &bufferInfos)
    {
        uint32_t totWriteCnt = (int)LightType::LIGHT_TYPE_CNT + 2;
        descriptorSetWrites.insert(descriptorSetWrites.end(), totWriteCnt, VkWriteDescriptorSet{});
        bufferInfos.resize(totWriteCnt);

        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            bufferInfos[i] = lightBuffers[i][id]->GetDescriptorBufferInfo();
            ConstructDescriptorSetWrite(descriptorSetWrites[i + 1], descriptorSet, i + 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &(bufferInfos[i]));
        }

        for (int i = (int)LightType::LIGHT_TYPE_CNT; i < totWriteCnt; ++i)
        {
            bufferInfos[i] = clusterBuffers[i - (int)LightType::LIGHT_TYPE_CNT][id]->GetDescriptorBufferInfo();
            ConstructDescriptorSetWrite(descriptorSetWrites[i + 1], descriptorSet, i + 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &(bufferInfos[i]));
        }
    }

    void LightManager::ConstructFrameGraph(FrameGraph &frameGraph,
                                           std::map<std::string, vke_ds::id32_t> &blackboard,
                                           ResourceNodeIDMap &currentResourceNodeID)
    {

        VkBuffer pointLightBuffers[MAX_FRAMES_IN_FLIGHT];
        VkBuffer spotLightBuffers[MAX_FRAMES_IN_FLIGHT];
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            pointLightBuffers[i] = clusterBuffers[0][i]->buffer;
            spotLightBuffers[i] = clusterBuffers[1][i]->buffer;
        }

        vke_ds::id32_t pointLightBufferResourceID = frameGraph.AddPermanentBufferResource("pointLightBuffer", true,
                                                                                          pointLightBuffers, 0, clusterBuffers[0][0]->bufferSize,
                                                                                          VK_PIPELINE_STAGE_NONE);
        vke_ds::id32_t spotLightBufferResourceID = frameGraph.AddPermanentBufferResource("spotLightBuffer", true,
                                                                                         spotLightBuffers, 0, clusterBuffers[1][0]->bufferSize,
                                                                                         VK_PIPELINE_STAGE_NONE);
        blackboard["pointLightClusterBuffer"] = pointLightBufferResourceID;
        blackboard["spotLightClusterBuffer"] = spotLightBufferResourceID;

        vke_ds::id32_t outPointLightBufferResourceNodeID = frameGraph.AllocResourceNode("outPointLight", pointLightBufferResourceID);
        vke_ds::id32_t outSpotLightBufferResourceNodeID = frameGraph.AllocResourceNode("outSpotLight", spotLightBufferResourceID);

        vke_ds::id32_t lightCullingTaskNodeID = frameGraph.AllocTaskNode("light culling", COMPUTE_TASK,
                                                                         std::bind(&LightManager::cullLights, this,
                                                                                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        frameGraph.AddTaskNodeResourceRef(lightCullingTaskNodeID, 0, outPointLightBufferResourceNodeID,
                                          VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                          VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        frameGraph.AddTaskNodeResourceRef(lightCullingTaskNodeID, 0, outSpotLightBufferResourceNodeID,
                                          VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                          VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        currentResourceNodeID[pointLightBufferResourceID] = outPointLightBufferResourceNodeID;
        currentResourceNodeID[spotLightBufferResourceID] = outSpotLightBufferResourceNodeID;
    }

    void LightManager::cullLights(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        lightCullingTask->Dispatch(commandBuffer,
                                   std::vector<VkDescriptorSet>{globalDescriptorSets[currentFrame]},
                                   lightCnts + 1,
                                   glm::ivec3(CLUSTER_DIM_X / 8, CLUSTER_DIM_Y / 8, CLUSTER_DIM_Z));
    }

    void LightManager::update(uint32_t currentFrame)
    {
        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            if (dirtyFlags[i])
            {
                lightUpdateCnts[i] = MAX_FRAMES_IN_FLIGHT;
                dirtyFlags[i] = false;
            }
        }

        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            if (lightUpdateCnts[i] > 0)
            {
                --lightUpdateCnts[i];
                RenderEnvironment::CopyBuffer(cpuLightBuffers[i]->buffer, lightBuffers[i][currentFrame]->buffer, cpuLightBuffers[i]->bufferSize);
            }
        }
    }

    DirectionalLight *LightManager::GetSun()
    {
        if (lightCnts[(int)LightType::DIRECTIONAL_LIGHT] == 0)
            return nullptr;

        return reinterpret_cast<DirectionalLight *>(cpuLightBuffers[(int)LightType::DIRECTIONAL_LIGHT]->data);
    }

    void LightManager::LoadSceneLightData(const SceneLightData &lighting)
    {
        ClearLights();

        for (auto &[entity, id] : lighting.entityToLight[(int)LightType::DIRECTIONAL_LIGHT])
        {
            VKE_FATAL_IF(id >= lighting.directionalLights.size(), "LIGHT DATA MAP OUT OF RANGE")
            AddLight<DirectionalLight>(entity, lighting.directionalLights[id]);
        }

        for (auto &[entity, id] : lighting.entityToLight[(int)LightType::POINT_LIGHT])
        {
            VKE_FATAL_IF(id >= lighting.pointLights.size(), "LIGHT DATA MAP OUT OF RANGE")
            AddLight<PointLight>(entity, lighting.pointLights[id]);
        }

        for (auto &[entity, id] : lighting.entityToLight[(int)LightType::SPOT_LIGHT])
        {
            VKE_FATAL_IF(id >= lighting.spotLights.size(), "LIGHT DATA MAP OUT OF RANGE")
            AddLight<SpotLight>(entity, lighting.spotLights[id]);
        }
    }

    SceneLightData LightManager::ToSceneLightData() const
    {
        SceneLightData lighting;

        const auto *directionalLights = reinterpret_cast<const DirectionalLight *>(cpuLightBuffers[(int)LightType::DIRECTIONAL_LIGHT]->data);
        lighting.directionalLights.assign(directionalLights, directionalLights + lightCnts[(int)LightType::DIRECTIONAL_LIGHT]);

        const auto *pointLights = reinterpret_cast<const PointLight *>(cpuLightBuffers[(int)LightType::POINT_LIGHT]->data);
        lighting.pointLights.assign(pointLights, pointLights + lightCnts[(int)LightType::POINT_LIGHT]);

        const auto *spotLights = reinterpret_cast<const SpotLight *>(cpuLightBuffers[(int)LightType::SPOT_LIGHT]->data);
        lighting.spotLights.assign(spotLights, spotLights + lightCnts[(int)LightType::SPOT_LIGHT]);

        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
            lighting.entityToLight[i] = entityToLight[i];

        return lighting;
    }

    void LightManager::ClearLights()
    {
        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            lightCnts[i] = 0;
            lightUpdateCnts[i] = MAX_FRAMES_IN_FLIGHT;
            dirtyFlags[i] = true;
            entityToLight[i].clear();
            ownerMaps[i].clear();
        }
    }
}
