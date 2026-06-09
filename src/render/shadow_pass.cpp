#include <render/shadow_pass.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

namespace vke_render
{
    static const std::vector<uint32_t> shadowVertexAttributeSizes = {
        sizeof(vke_render::Vertex::pos),
        sizeof(vke_render::Vertex::normal),
        sizeof(vke_render::Vertex::tangent),
        sizeof(vke_render::Vertex::texCoord)};

    static const std::vector<uint32_t> shadowSkinVertexAttributeSizes = {
        sizeof(vke_render::SkinVertex::pos),
        sizeof(vke_render::SkinVertex::normal),
        sizeof(vke_render::SkinVertex::tangent),
        sizeof(vke_render::SkinVertex::texCoord),
        sizeof(vke_render::SkinVertex::weights),
        sizeof(vke_render::SkinVertex::jointIDs),
    };

    struct ShadowPushConstants
    {
        glm::mat4 model;
        uint32_t cascadeIndex;
    };

    static float CalculateCascadeSplit(float cameraNear, float clipRange, float cameraFar, float splitProgress, float splitLambda)
    {
        float logSplit = cameraNear * std::pow(cameraFar / cameraNear, splitProgress);
        float uniformSplit = cameraNear + clipRange * splitProgress;
        return splitLambda * logSplit + (1.0f - splitLambda) * uniformSplit;
    }

    static std::array<glm::vec3, 8> CalculateCascadeCorners(const CameraInfo &cam, float nearClip, float farClip)
    {
        float tanHalfFov = std::tan(cam.fov * 0.5f);
        float nearHalfHeight = nearClip * tanHalfFov;
        float nearHalfWidth = nearHalfHeight * cam.aspect;
        float farHalfHeight = farClip * tanHalfFov;
        float farHalfWidth = farHalfHeight * cam.aspect;

        std::array<glm::vec3, 8> viewCorners = {
            glm::vec3(-nearHalfWidth, -nearHalfHeight, -nearClip),
            glm::vec3(-nearHalfWidth, nearHalfHeight, -nearClip),
            glm::vec3(nearHalfWidth, -nearHalfHeight, -nearClip),
            glm::vec3(nearHalfWidth, nearHalfHeight, -nearClip),
            glm::vec3(-farHalfWidth, -farHalfHeight, -farClip),
            glm::vec3(-farHalfWidth, farHalfHeight, -farClip),
            glm::vec3(farHalfWidth, -farHalfHeight, -farClip),
            glm::vec3(farHalfWidth, farHalfHeight, -farClip),
        };

        std::array<glm::vec3, 8> worldCorners;
        for (uint32_t i = 0; i < viewCorners.size(); ++i)
            worldCorners[i] = glm::vec3(cam.invView * glm::vec4(viewCorners[i], 1.0f));
        return worldCorners;
    }

    static float CalculateStableCascadeRadius(float fov, float aspect, float nearClip, float farClip)
    {
        float tanHalfFov = std::tan(fov * 0.5f);
        float nearHalfHeight = nearClip * tanHalfFov;
        float nearHalfWidth = nearHalfHeight * aspect;
        float farHalfHeight = farClip * tanHalfFov;
        float farHalfWidth = farHalfHeight * aspect;
        float centerZ = -(nearClip + farClip) * 0.5f;

        std::array<glm::vec3, 8> viewCorners = {
            glm::vec3(-nearHalfWidth, -nearHalfHeight, -nearClip),
            glm::vec3(-nearHalfWidth, nearHalfHeight, -nearClip),
            glm::vec3(nearHalfWidth, -nearHalfHeight, -nearClip),
            glm::vec3(nearHalfWidth, nearHalfHeight, -nearClip),
            glm::vec3(-farHalfWidth, -farHalfHeight, -farClip),
            glm::vec3(-farHalfWidth, farHalfHeight, -farClip),
            glm::vec3(farHalfWidth, -farHalfHeight, -farClip),
            glm::vec3(farHalfWidth, farHalfHeight, -farClip),
        };

        glm::vec3 center(0.0f, 0.0f, centerZ);
        float radius = 0.0f;
        for (const glm::vec3 &corner : viewCorners)
            radius = std::max(radius, glm::length(corner - center));
        return std::ceil(radius * 16.0f) / 16.0f;
    }

    ShadowPass::~ShadowPass()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (shadowMapImageViews[i] != VK_NULL_HANDLE)
            {
                vkDestroyImageView(globalLogicalDevice, shadowMapImageViews[i], nullptr);
                shadowMapImageViews[i] = VK_NULL_HANDLE;
            }
            for (uint32_t cascade = 0; cascade < MAX_DIRECTIONAL_SHADOW_CASCADE_CNT; ++cascade)
            {
                if (shadowCascadeImageViews[i][cascade] != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(globalLogicalDevice, shadowCascadeImageViews[i][cascade], nullptr);
                    shadowCascadeImageViews[i][cascade] = VK_NULL_HANDLE;
                }
            }
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            if (shadowMapImages[i] != VK_NULL_HANDLE)
            {
                vkDestroyImage(globalLogicalDevice, shadowMapImages[i], nullptr);
                shadowMapImages[i] = VK_NULL_HANDLE;
            }

        if (shadowMapSampler != VK_NULL_HANDLE)
            vkDestroySampler(globalLogicalDevice, shadowMapSampler, nullptr);
    }

    void ShadowPass::Init(int subpassID,
                          FrameGraph &frameGraph,
                          std::map<std::string, vke_ds::id32_t> &blackboard,
                          ResourceNodeIDMap &currentResourceNodeID)
    {
        RenderPassBase::Init(subpassID, frameGraph, blackboard, currentResourceNodeID);

        shadowMaterial = std::make_shared<Material>(vke_common::BUILTIN_VFSHADER_SHADOW_ID);
        shadowMaterial->shader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_SHADOW_ID);
        shadowMaterial->textureBindingInfos = std::make_shared<std::vector<TextureBindingInfo>>();
        shadowSkinMaterial = std::make_shared<Material>(vke_common::BUILTIN_VFSHADER_SHADOW_SKIN_ID);
        shadowSkinMaterial->shader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_SHADOW_SKIN_ID);
        shadowSkinMaterial->textureBindingInfos = std::make_shared<std::vector<TextureBindingInfo>>();
        registerMaterial(shadowMaterial, false);
        registerMaterial(shadowSkinMaterial, true);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            shadowInfoBuffers.emplace_back(sizeof(DirectionalShadowInfoCPU), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        createImages();
        createSampler();
        constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
        allocateDescriptorSets();
    }

    vke_ds::id64_t ShadowPass::AddUnit(RenderUnit *unit, bool isSkin)
    {
        std::shared_ptr<Material> &material = isSkin ? shadowSkinMaterial : shadowMaterial;
        registerMaterial(material, isSkin);
        vke_ds::id64_t id = unitAllocator.Alloc();
        renderInfoMap[material.get()]->units[id] = unit;
        unitMaterialMap[id] = material.get();
        return id;
    }

    void ShadowPass::RemoveUnit(vke_ds::id64_t id)
    {
        auto materialIt = unitMaterialMap.find(id);
        if (materialIt == unitMaterialMap.end())
            return;

        auto renderInfoIt = renderInfoMap.find(materialIt->second);
        if (renderInfoIt != renderInfoMap.end())
            renderInfoIt->second->RemoveUnit(id);
        unitMaterialMap.erase(materialIt);
    }

    void ShadowPass::constructFrameGraph(FrameGraph &frameGraph,
                                         std::map<std::string, vke_ds::id32_t> &blackboard,
                                         ResourceNodeIDMap &currentResourceNodeID)
    {
        shadowMapResourceID = frameGraph.AddTransientImageResource("directionalShadowMap0", shadowMapImages, VK_IMAGE_ASPECT_DEPTH_BIT, 1, config.cascadeCnt);
        shadowMapResourceNodeID = frameGraph.AllocResourceNode("directionalShadowMap0", shadowMapResourceID);
        vke_ds::id32_t shadowMapOutResourceNodeID = frameGraph.AllocResourceNode("directionalShadowMap0Out", shadowMapResourceID);

        blackboard["directionalShadowMap0"] = shadowMapResourceID;
        blackboard["directionalShadowMap0OutNode"] = shadowMapOutResourceNodeID; // TODO no out node
        currentResourceNodeID[shadowMapResourceID] = shadowMapResourceNodeID;

        shadowTaskNodeID = frameGraph.AllocTaskNode("shadow pass", RENDER_TASK,
                                                    std::bind(&ShadowPass::Render, this,
                                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        frameGraph.SetTaskNodeTransientReadyCallback(shadowTaskNodeID,
                                                     std::bind(&ShadowPass::onTransientResourcesReady, this,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        frameGraph.AddTaskNodeResourceRef(shadowTaskNodeID, shadowMapResourceNodeID, shadowMapOutResourceNodeID,
                                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        frameGraph.AddTargetResource(shadowMapResourceID);
        currentResourceNodeID[shadowMapResourceID] = shadowMapOutResourceNodeID;
    }

    void ShadowPass::createImages()
    {
        VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            RenderEnvironment::CreateImageWithoutMemory(config.mapSize, config.mapSize,
                                                        context->depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                                        usageFlags, 1, &shadowMapImages[i], config.cascadeCnt);
    }

    void ShadowPass::createImageViews(uint32_t currentFrame)
    {
        if (shadowMapImageViews[currentFrame] != VK_NULL_HANDLE)
            vkDestroyImageView(globalLogicalDevice, shadowMapImageViews[currentFrame], nullptr);
        shadowMapImageViews[currentFrame] = RenderEnvironment::CreateImageView(shadowMapImages[currentFrame], context->depthFormat,
                                                                               VK_IMAGE_ASPECT_DEPTH_BIT, 1, 0,
                                                                               config.cascadeCnt, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
        for (uint32_t cascade = 0; cascade < config.cascadeCnt; ++cascade)
        {
            if (shadowCascadeImageViews[currentFrame][cascade] != VK_NULL_HANDLE)
                vkDestroyImageView(globalLogicalDevice, shadowCascadeImageViews[currentFrame][cascade], nullptr);
            shadowCascadeImageViews[currentFrame][cascade] = RenderEnvironment::CreateImageView(shadowMapImages[currentFrame],
                                                                                                context->depthFormat,
                                                                                                VK_IMAGE_ASPECT_DEPTH_BIT,
                                                                                                1, cascade, 1,
                                                                                                VK_IMAGE_VIEW_TYPE_2D);
        }
    }

    void ShadowPass::createSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        VKE_VK_CHECK(vkCreateSampler(globalLogicalDevice, &samplerInfo, nullptr, &shadowMapSampler), "failed to create shadow map sampler!")
    }

    void ShadowPass::allocateDescriptorSets()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            shadowDescriptorSets[i] = shadowMaterial->shader->CreateDescriptorSet(0);
            VkDescriptorBufferInfo bufferInfo = shadowInfoBuffers[i].GetDescriptorBufferInfo();
            VkWriteDescriptorSet descriptorWrite{};
            ConstructDescriptorSetWrite(descriptorWrite, shadowDescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
            vkUpdateDescriptorSets(globalLogicalDevice, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void ShadowPass::createGraphicsPipeline(RenderInfo &renderInfo, bool isSkin)
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.colorAttachmentCount = 0;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = nullptr;
        pipelineRenderingCreateInfo.depthAttachmentFormat = context->depthFormat;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.25f;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pRasterizationState = &rasterizer;

        renderInfo.CreatePipeline(isSkin ? shadowSkinVertexAttributeSizes : shadowVertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
    }

    void ShadowPass::registerMaterial(std::shared_ptr<Material> &material, bool isSkin)
    {
        Material *matp = material.get();
        if (renderInfoMap.find(matp) != renderInfoMap.end())
            return;

        std::unique_ptr<RenderInfo> info = std::make_unique<RenderInfo>(material);
        createGraphicsPipeline(*info, isSkin);
        renderInfoMap[matp] = std::move(info);
    }

    void ShadowPass::updateShadowInfo(uint32_t currentFrame)
    {
        DirectionalLight *sun = lightManager->GetSun();
        if (sun == nullptr)
        {
            directionalShadowInfo = DirectionalShadowInfoCPU();
            shadowInfoBuffers[currentFrame].ToBuffer(0, &directionalShadowInfo, sizeof(DirectionalShadowInfoCPU));
            return;
        }

        glm::vec3 lightDir = glm::normalize(glm::vec3(sun->direction));
        if (glm::length(lightDir) < 0.0001f)
            lightDir = glm::vec3(0.0f, -1.0f, 0.0f);

        const CameraInfo &cam = *cameraInfo;
        const float cameraNear = std::max(cam.near, 0.01f);
        const float cameraFar = std::max(cameraNear + 0.01f, std::min(cam.far, config.maxDistance));
        const float clipRange = cameraFar - cameraNear;

        float lastSplitDist = 0.0f;
        glm::vec3 up = std::abs(glm::dot(lightDir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.98f
                           ? glm::vec3(0.0f, 0.0f, 1.0f)
                           : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 lightRight = glm::normalize(glm::cross(lightDir, up));
        glm::vec3 lightUp = glm::normalize(glm::cross(lightRight, lightDir));

        directionalShadowInfo.lightIndex = glm::uvec4(0, 0, 0, 0);
        directionalShadowInfo.cascadeCnt = glm::uvec4(config.cascadeCnt, 0, 0, 0);
        directionalShadowInfo.cascadeSplits = glm::vec4(0.0f);
        directionalShadowInfo.cascadeTexelSizes = glm::vec4(0.0f);

        for (uint32_t cascade = 0; cascade < config.cascadeCnt; ++cascade)
        {
            float p = static_cast<float>(cascade + 1) / static_cast<float>(config.cascadeCnt);
            float split = CalculateCascadeSplit(cameraNear, clipRange, cameraFar, p, config.splitLambda);
            float splitDist = (split - cameraNear) / clipRange;
            float nearClip = cameraNear + clipRange * lastSplitDist;
            float farClip = cameraNear + clipRange * splitDist;
            std::array<glm::vec3, 8> frustumCorners = CalculateCascadeCorners(cam, nearClip, farClip);

            glm::vec3 center(0.0f);
            for (const glm::vec3 &corner : frustumCorners)
                center += corner;
            center /= static_cast<float>(frustumCorners.size());

            float radius = CalculateStableCascadeRadius(cam.fov, cam.aspect, nearClip, farClip);

            float worldUnitsPerTexel = (radius * 2.0f) / static_cast<float>(config.mapSize);
            directionalShadowInfo.cascadeTexelSizes[cascade] = worldUnitsPerTexel;
            float centerRight = glm::dot(center, lightRight);
            float centerUp = glm::dot(center, lightUp);
            float snappedRight = std::round(centerRight / worldUnitsPerTexel) * worldUnitsPerTexel;
            float snappedUp = std::round(centerUp / worldUnitsPerTexel) * worldUnitsPerTexel;
            glm::vec3 snappedCenter = center +
                                      lightRight * (snappedRight - centerRight) +
                                      lightUp * (snappedUp - centerUp);
            float lightDistance = radius + config.depthMargin;
            glm::mat4 lightView = glm::lookAt(snappedCenter - lightDir * lightDistance,
                                              snappedCenter, lightUp);

            float lightNear = 0.0f;
            float lightFar = lightDistance + radius + config.depthMargin;
            glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, lightNear, lightFar);
            lightProj[1][1] *= -1.0f;

            directionalShadowInfo.cascadeSplits[cascade] = split;
            directionalShadowInfo.lightViewProj[cascade] = lightProj * lightView;
            lastSplitDist = splitDist;
        }

        shadowInfoBuffers[currentFrame].ToBuffer(0, &directionalShadowInfo, sizeof(DirectionalShadowInfoCPU));
    }

    void ShadowPass::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        updateShadowInfo(currentFrame);

        if (directionalShadowInfo.lightIndex.x != INVALID_SHADOW_LIGHT_INDEX)
        {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(config.mapSize);
            viewport.height = static_cast<float>(config.mapSize);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {config.mapSize, config.mapSize};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            for (uint32_t cascade = 0; cascade < config.cascadeCnt; ++cascade)
            {
                VkRenderingAttachmentInfo depthAttachmentInfo{};
                depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAttachmentInfo.pNext = nullptr;
                depthAttachmentInfo.imageView = shadowCascadeImageViews[currentFrame][cascade];
                depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthAttachmentInfo.clearValue.depthStencil = {1.0f, 0};
                depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;

                VkRenderingInfo renderingInfo{};
                renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                renderingInfo.pNext = nullptr;
                renderingInfo.renderArea = {{0, 0}, {config.mapSize, config.mapSize}};
                renderingInfo.layerCount = 1;
                renderingInfo.colorAttachmentCount = 0;
                renderingInfo.pColorAttachments = nullptr;
                renderingInfo.pDepthAttachment = &depthAttachmentInfo;

                vkCmdBeginRendering(commandBuffer, &renderingInfo);

                for (auto &kv : renderInfoMap)
                {
                    std::unique_ptr<RenderInfo> &renderInfo = kv.second;
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo->renderPipeline->pipeline);
                    vkCmdPushConstants(commandBuffer, renderInfo->renderPipeline->pipelineLayout, VK_SHADER_STAGE_ALL,
                                       offsetof(ShadowPushConstants, cascadeIndex), sizeof(uint32_t), &cascade);
                    renderInfo->Render(commandBuffer, shadowDescriptorSets[currentFrame]);
                }

                vkCmdEndRendering(commandBuffer);
            }
        }
    }

    void ShadowPass::OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx)
    {
    }

    void ShadowPass::onTransientResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame)
    {
        createImageViews(currentFrame);
    }
}
