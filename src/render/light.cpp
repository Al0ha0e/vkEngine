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
        }
        sceneLighting = nullptr;

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
                                           std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
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

        vke_ds::id32_t outPointLightBufferResourceNodeID = frameGraph.AllocResourceNode("outPointLight", false, pointLightBufferResourceID);
        vke_ds::id32_t outSpotLightBufferResourceNodeID = frameGraph.AllocResourceNode("outSpotLight", false, spotLightBufferResourceID);

        vke_ds::id32_t lightCullingTaskNodeID = frameGraph.AllocTaskNode("light culling", COMPUTE_TASK,
                                                                         std::bind(&LightManager::cullLights, this,
                                                                                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        frameGraph.AddTaskNodeResourceRef(lightCullingTaskNodeID, false, 0, outPointLightBufferResourceNodeID,
                                          VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                          VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        frameGraph.AddTaskNodeResourceRef(lightCullingTaskNodeID, false, 0, outSpotLightBufferResourceNodeID,
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
        if (sceneLighting == nullptr)
            return;

        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            if (sceneLighting->dirtyFlags[i])
            {
                lightCnts[i] = sceneLighting->lightCnts[i];
                lightUpdateCnts[i] = MAX_FRAMES_IN_FLIGHT;
                sceneLighting->dirtyFlags[i] = false;
            }
        }

        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            if (lightUpdateCnts[i] > 0)
            {
                --lightUpdateCnts[i];
                if (sceneLighting != nullptr)
                    RenderEnvironment::CopyBuffer(sceneLighting->cpuLightBuffers[i]->buffer, lightBuffers[i][currentFrame]->buffer, sceneLighting->cpuLightBuffers[i]->bufferSize);
            }
        }
    }

    void LightManager::BindSceneLighting(SceneLighting *lighting)
    {
        sceneLighting = lighting;
        for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
        {
            lightCnts[i] = sceneLighting == nullptr ? 0 : sceneLighting->lightCnts[i];
            lightUpdateCnts[i] = MAX_FRAMES_IN_FLIGHT;
        }

        if (sceneLighting != nullptr)
            for (int i = 0; i < (int)LightType::LIGHT_TYPE_CNT; ++i)
                sceneLighting->dirtyFlags[i] = true;
    }
}
