#include <render/shadow_manager.hpp>
#include <render/light.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

namespace vke_render
{
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

    ShadowManager::ShadowManager(RenderContext *ctx, FrameGraph &frameGraph, std::shared_ptr<const CPULightData> cpuLightData, const CameraInfo *cameraInfo,
                                 const DirectionalShadowConfig &config)
        : context(ctx), cpuLightData(cpuLightData), cameraInfo(cameraInfo), config(config),
          shadowMapSampler(VK_NULL_HANDLE)
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            shadowInfoBuffers[i] = std::make_unique<HostCoherentBuffer>(sizeof(DirectionalShadowInfoCPU), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            shadowPassDescriptorSets[i] = VK_NULL_HANDLE;
            deferredLightingDescriptorSets[i] = VK_NULL_HANDLE;
            shadowMapImages[i] = VK_NULL_HANDLE;
            shadowMapImageViews[i] = VK_NULL_HANDLE;
            for (uint32_t cascade = 0; cascade < MAX_DIRECTIONAL_SHADOW_CASCADE_CNT; ++cascade)
                shadowCascadeImageViews[i][cascade] = VK_NULL_HANDLE;
        }

        createImages();
        createSampler();
        createDescriptorSets();
        UpdateShadow();
        frameGraph.AddTransientReadyCallback(std::bind(&ShadowManager::onTransientResourcesReady, this, std::placeholders::_1));
    }

    ShadowManager::~ShadowManager()
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

            if (shadowMapImages[i] != VK_NULL_HANDLE)
            {
                vkDestroyImage(globalLogicalDevice, shadowMapImages[i], nullptr);
                shadowMapImages[i] = VK_NULL_HANDLE;
            }
        }

        if (shadowMapSampler != VK_NULL_HANDLE)
            vkDestroySampler(globalLogicalDevice, shadowMapSampler, nullptr);
    }

    void ShadowManager::createDescriptorSets()
    {
        std::shared_ptr<ShaderModuleSet> shadowShader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_SHADOW_ID);
        std::shared_ptr<ShaderModuleSet> lightingShader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_DEFERRED_LIGHTING_ID);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            shadowPassDescriptorSets[i] = shadowShader->CreateDescriptorSet(0);
            deferredLightingDescriptorSets[i] = lightingShader->CreateDescriptorSet(2);
            updateShadowPassDescriptorSet(i);
        }
    }

    void ShadowManager::SyncToGPU(uint32_t currentFrame)
    {
        shadowInfoBuffers[currentFrame]->ToBuffer(0, &directionalShadowInfo, sizeof(DirectionalShadowInfoCPU));
    }

    void ShadowManager::createImages()
    {
        VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            RenderEnvironment::CreateImageWithoutMemory(config.mapSize, config.mapSize,
                                                        context->depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                                        usageFlags, 1, &shadowMapImages[i], config.cascadeCnt);
    }

    void ShadowManager::createImageViews(uint32_t currentFrame)
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

    void ShadowManager::createSampler()
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

    void ShadowManager::updateShadowPassDescriptorSet(uint32_t currentFrame)
    {
        VkDescriptorBufferInfo bufferInfo = shadowInfoBuffers[currentFrame]->GetDescriptorBufferInfo();
        VkWriteDescriptorSet descriptorWrite{};
        ConstructDescriptorSetWrite(descriptorWrite, shadowPassDescriptorSets[currentFrame], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        vkUpdateDescriptorSets(globalLogicalDevice, 1, &descriptorWrite, 0, nullptr);
    }

    void ShadowManager::updateDeferredLightingDescriptorSet(uint32_t currentFrame)
    {
        if (deferredLightingDescriptorSets[currentFrame] == VK_NULL_HANDLE)
            return;

        VkWriteDescriptorSet descriptorWrites[2];
        VkDescriptorBufferInfo shadowBufferInfo = shadowInfoBuffers[currentFrame]->GetDescriptorBufferInfo();
        ConstructDescriptorSetWrite(descriptorWrites[0], deferredLightingDescriptorSets[currentFrame], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &shadowBufferInfo);

        VkDescriptorImageInfo shadowImageInfo = {shadowMapSampler, shadowMapImageViews[currentFrame], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        ConstructDescriptorSetWrite(descriptorWrites[1], deferredLightingDescriptorSets[currentFrame], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &shadowImageInfo);

        vkUpdateDescriptorSets(globalLogicalDevice, 2, descriptorWrites, 0, nullptr);
    }

    void ShadowManager::UpdateShadow()
    {
        if (cpuLightData->lightCnts[(int)LightType::DIRECTIONAL_LIGHT] == 0)
        {
            directionalShadowInfo = DirectionalShadowInfoCPU();
            return;
        }

        const DirectionalLight *sun = reinterpret_cast<const DirectionalLight *>(
            cpuLightData->cpuLightBuffers[(int)LightType::DIRECTIONAL_LIGHT]->data);
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
    }

    void ShadowManager::onTransientResourcesReady(uint32_t currentFrame)
    {
        createImageViews(currentFrame);
        updateDeferredLightingDescriptorSet(currentFrame);
    }
}
