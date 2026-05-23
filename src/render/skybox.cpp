#include <render/skybox.hpp>
#include <asset.hpp>

namespace vke_render
{
    namespace
    {
        const glm::vec4 DEFAULT_SUN_LIGHT_DIR = glm::normalize(glm::vec4(1, 2, 0, 0));
    }

    void SkyboxManager::constructFrameGraph(FrameGraph &frameGraph,
                                            std::map<std::string, vke_ds::id32_t> &blackboard,
                                            std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        vke_ds::id32_t lutTaskID = frameGraph.AllocTaskNode("gen sky lut", COMPUTE_TASK,
                                                            std::bind(&SkyboxManager::generateLUT, this,
                                                                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        vke_ds::id32_t iblTaskID = frameGraph.AllocTaskNode("gen sky ibl lut", COMPUTE_TASK,
                                                            std::bind(&SkyboxManager::generateIBLLUT, this,
                                                                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        VkImage skyLUTImages[MAX_FRAMES_IN_FLIGHT];
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            skyLUTImages[i] = skyLUTs[i]->image;
        vke_ds::id32_t lutResourceID = frameGraph.AddPermanentImageResource("skyLUT", true, skyLUTImages,
                                                                            VK_IMAGE_ASPECT_COLOR_BIT, skyLUTs[0]->mipLevelCnt, 6, false,
                                                                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, std::nullopt);
        vke_ds::id32_t lutOutResourceNodeID = frameGraph.AllocResourceNode("outSkyLUT", false, lutResourceID);

        VkImage skyIrradianceImages[MAX_FRAMES_IN_FLIGHT];
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            skyIrradianceImages[i] = skyIrradianceLUTs[i]->image;
        vke_ds::id32_t irradianceResourceID = frameGraph.AddPermanentImageResource("skyIrradianceLUT", true, skyIrradianceImages,
                                                                                   VK_IMAGE_ASPECT_COLOR_BIT, skyIrradianceLUTs[0]->mipLevelCnt, 6, false,
                                                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, std::nullopt);
        vke_ds::id32_t irradianceOutResourceNodeID = frameGraph.AllocResourceNode("outSkyIrradianceLUT", false, irradianceResourceID);

        VkImage skySpecularImages[MAX_FRAMES_IN_FLIGHT];
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            skySpecularImages[i] = skySpecularLUTs[i]->image;
        vke_ds::id32_t specularResourceID = frameGraph.AddPermanentImageResource("skySpecularLUT", true, skySpecularImages,
                                                                                 VK_IMAGE_ASPECT_COLOR_BIT, skySpecularLUTs[0]->mipLevelCnt, 6, false,
                                                                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, std::nullopt);
        vke_ds::id32_t specularOutResourceNodeID = frameGraph.AllocResourceNode("outSkySpecularLUT", false, specularResourceID);

        frameGraph.AddTaskNodeResourceRef(lutTaskID, false, 0, lutOutResourceNodeID,
                                          VK_ACCESS_SHADER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_GENERAL);
        frameGraph.AddTaskNodeResourceRef(iblTaskID, false, lutOutResourceNodeID, 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        frameGraph.AddTaskNodeResourceRef(iblTaskID, false, 0, irradianceOutResourceNodeID,
                                          VK_ACCESS_SHADER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_GENERAL);
        frameGraph.AddTaskNodeResourceRef(iblTaskID, false, 0, specularOutResourceNodeID,
                                          VK_ACCESS_SHADER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_GENERAL);

        blackboard["skyLUT"] = lutResourceID;
        blackboard["skyIrradianceLUT"] = irradianceResourceID;
        blackboard["skySpecularLUT"] = specularResourceID;
        currentResourceNodeID[lutResourceID] = lutOutResourceNodeID;
        currentResourceNodeID[irradianceResourceID] = irradianceOutResourceNodeID;
        currentResourceNodeID[specularResourceID] = specularOutResourceNodeID;
    }

    void SkyboxManager::initResources()
    {
        const auto &paramJSON = vke_common::AssetManager::LoadJSON(std::string(REL_DIR) + "/builtin_assets/config/atmosphere_param.json");
        atmosphereParameter = AtmosphereParameter(paramJSON);
        atmosphereParamBuffer = std::make_unique<DeviceBuffer>(sizeof(AtmosphereParameter), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        atmosphereParamBuffer->ToBuffer(0, &atmosphereParameter, sizeof(AtmosphereParameter));

        std::shared_ptr<ShaderModuleSet> skyLUTShader = vke_common::AssetManager::LoadComputeShaderUnique(vke_common::BUILTIN_COMPUTE_SHADER_SKYLUT_ID);
        skyLUTGenerationPipeline = std::make_unique<ComputePipeline>(std::move(skyLUTShader));
        std::shared_ptr<ShaderModuleSet> iblLUTShader = vke_common::AssetManager::LoadComputeShaderUnique(vke_common::BUILTIN_COMPUTE_SHADER_IBL_LUT_ID);
        iblLUTGenerationPipeline = std::make_unique<ComputePipeline>(std::move(iblLUTShader));
        skyboxMaterial = vke_common::AssetManager::LoadMaterial(vke_common::BUILTIN_MATERIAL_SKYBOX_ID);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            skyLUTs[i] = std::make_unique<CubeMap>(SKY_VIEW_CUBE_SIZE, SKY_VIEW_CUBE_SIZE,
                                                   VK_FORMAT_R16G16B16A16_SFLOAT,
                                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                                   VK_IMAGE_LAYOUT_GENERAL,
                                                   VK_FILTER_LINEAR,
                                                   VK_FILTER_LINEAR,
                                                   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                   false);
            skyIrradianceLUTs[i] = std::make_unique<CubeMap>(SKY_IRRADIANCE_CUBE_SIZE, SKY_IRRADIANCE_CUBE_SIZE,
                                                             VK_FORMAT_R16G16B16A16_SFLOAT,
                                                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                                             VK_IMAGE_LAYOUT_GENERAL,
                                                             VK_FILTER_LINEAR,
                                                             VK_FILTER_LINEAR,
                                                             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                             false);
            skySpecularLUTs[i] = std::make_unique<CubeMap>(SKY_SPECULAR_CUBE_SIZE, SKY_SPECULAR_CUBE_SIZE,
                                                           VK_FORMAT_R16G16B16A16_SFLOAT,
                                                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                                           VK_IMAGE_LAYOUT_GENERAL,
                                                           VK_FILTER_LINEAR,
                                                           VK_FILTER_LINEAR,
                                                           VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                           false,
                                                           SKY_SPECULAR_MIP_LEVELS);
        }
    }

    void SkyboxManager::createDescriptorSet()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            skyLUTDescriptorSets[i] = skyLUTGenerationPipeline->shader->CreateDescriptorSet(1);
            skyboxMaterial->UpdateDescriptorSet(skyLUTDescriptorSets[i]);
            skyIBLDescriptorSets[i] = iblLUTGenerationPipeline->shader->CreateDescriptorSet(0);
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VkWriteDescriptorSet skyDescriptorWrites[3];
            VkDescriptorBufferInfo bufferInfo = atmosphereParamBuffer->GetDescriptorBufferInfo();
            vke_render::ConstructDescriptorSetWrite(skyDescriptorWrites[0], skyLUTDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
            VkDescriptorImageInfo skyReadInfo{skyLUTs[i]->sampler, skyLUTs[i]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            vke_render::ConstructDescriptorSetWrite(skyDescriptorWrites[1], skyLUTDescriptorSets[i], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &skyReadInfo);
            VkDescriptorImageInfo skyWriteInfo{nullptr, skyLUTs[i]->imageView, VK_IMAGE_LAYOUT_GENERAL};
            vke_render::ConstructDescriptorSetWrite(skyDescriptorWrites[2], skyLUTDescriptorSets[i], 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &skyWriteInfo);
            vkUpdateDescriptorSets(globalLogicalDevice, 3, skyDescriptorWrites, 0, nullptr);

            VkWriteDescriptorSet iblDescriptorWrites[3];
            VkDescriptorImageInfo skyViewInfo{skyLUTs[i]->sampler, skyLUTs[i]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            vke_render::ConstructDescriptorSetWrite(iblDescriptorWrites[0], skyIBLDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &skyViewInfo);
            VkDescriptorImageInfo irradianceImageInfo{nullptr, skyIrradianceLUTs[i]->imageView, VK_IMAGE_LAYOUT_GENERAL};
            vke_render::ConstructDescriptorSetWrite(iblDescriptorWrites[1], skyIBLDescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &irradianceImageInfo);

            VkDescriptorImageInfo specularImageInfos[SKY_SPECULAR_MIP_LEVELS];
            for (uint32_t mip = 0; mip < SKY_SPECULAR_MIP_LEVELS; ++mip)
                specularImageInfos[mip] = {nullptr, skySpecularLUTs[i]->GetMipImageView(mip), VK_IMAGE_LAYOUT_GENERAL};
            vke_render::ConstructDescriptorSetWrite(iblDescriptorWrites[2], skyIBLDescriptorSets[i], 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, specularImageInfos, 0, SKY_SPECULAR_MIP_LEVELS);
            vkUpdateDescriptorSets(globalLogicalDevice, 3, iblDescriptorWrites, 0, nullptr);
        }
    }

    void SkyboxManager::updateSunLightDir()
    {
        DirectionalLight *sun = lightManager == nullptr ? nullptr : lightManager->GetSun();
        sunLightDir = sun == nullptr ? DEFAULT_SUN_LIGHT_DIR : -glm::normalize(sun->direction);
    }

    void SkyboxManager::generateLUT(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        updateSunLightDir();
        skyLUTGenerationPipeline->Dispatch(commandBuffer, std::vector<VkDescriptorSet>{globalDescriptorSets[currentFrame], skyLUTDescriptorSets[currentFrame]},
                                           &sunLightDir, glm::ivec3((SKY_VIEW_CUBE_SIZE + 31) / 32, (SKY_VIEW_CUBE_SIZE + 31) / 32, 6));
    }

    void SkyboxManager::generateIBLLUT(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        iblLUTGenerationPipeline->Dispatch(commandBuffer,
                                           std::vector<VkDescriptorSet>{skyIBLDescriptorSets[currentFrame]},
                                           glm::ivec3((SKY_SPECULAR_CUBE_SIZE + 7) / 8, (SKY_SPECULAR_CUBE_SIZE + 7) / 8, 6));
    }
}
