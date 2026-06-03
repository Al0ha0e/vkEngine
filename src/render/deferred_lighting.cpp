#include <render/deferred_lighting.hpp>
#include <asset.hpp>

namespace vke_render
{

    void DeferredLightingPass::constructFrameGraph(FrameGraph &frameGraph,
                                                   std::map<std::string, vke_ds::id32_t> &blackboard,
                                                   std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        vke_ds::id32_t colorAttachmentResourceID = blackboard["colorAttachment"];
        vke_ds::id32_t irradianceResourceID = blackboard.at("skyIrradianceLUT");
        vke_ds::id32_t specularResourceID = blackboard.at("skySpecularLUT");
        vke_ds::id32_t irradianceOutResourceNodeID = currentResourceNodeID[irradianceResourceID];
        vke_ds::id32_t specularOutResourceNodeID = currentResourceNodeID[specularResourceID];

        vke_ds::id32_t lightingOutColorResourceNodeID = frameGraph.AllocResourceNode("deferredLightingOutColor", false, colorAttachmentResourceID);
        lightingTaskNodeID = frameGraph.AllocTaskNode("deferred lighting", RENDER_TASK,
                                                      std::bind(&DeferredLightingPass::Render, this,
                                                                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        frameGraph.SetTaskNodeTransientReadyCallback(lightingTaskNodeID,
                                                     std::bind(&DeferredLightingPass::onTransientResourcesReady, this,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, false, currentResourceNodeID[blackboard["pointLightClusterBuffer"]], 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, false, currentResourceNodeID[blackboard["spotLightClusterBuffer"]], 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        for (int i = 0; i < GBUFFER_CNT; i++)
            frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, true, gbuffer->GetResourceNodeID(i), 0,
                                              VK_ACCESS_SHADER_READ_BIT,
                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                              VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                              VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

        auto shadowMapNodeIt = blackboard.find("directionalShadowMap0OutNode");
        if (shadowMapNodeIt != blackboard.end())
            frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, true, shadowMapNodeIt->second, 0,
                                              VK_ACCESS_SHADER_READ_BIT,
                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                              VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, false, currentResourceNodeID[colorAttachmentResourceID], lightingOutColorResourceNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, false, irradianceOutResourceNodeID, 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        frameGraph.AddTaskNodeResourceRef(lightingTaskNodeID, false, specularOutResourceNodeID, 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        currentResourceNodeID[colorAttachmentResourceID] = lightingOutColorResourceNodeID;
    }

    void DeferredLightingPass::allocateDescriptorSet()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            lightingDescriptorSets[i] = lightingShader->CreateDescriptorSet(1);
            shadowDescriptorSets[i] = lightingShader->CreateDescriptorSet(2);
        }
    }

    void DeferredLightingPass::updateDescriptorSet(uint32_t currentFrame)
    {
        const uint32_t extraTextureCnt = 3;
        VkWriteDescriptorSet descriptorWrites[GBUFFER_CNT + extraTextureCnt];
        VkDescriptorImageInfo imageInfos[GBUFFER_CNT + extraTextureCnt];

        for (int i = 0; i < GBUFFER_CNT; ++i)
        {
            imageInfos[i] = {gbuffer->sampler, gbuffer->imageViews[i][currentFrame], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            vke_render::ConstructDescriptorSetWrite(descriptorWrites[i], lightingDescriptorSets[currentFrame], i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &(imageInfos[i]));
        }

        CubeMap *skyIrradianceLUT = skyboxManager->GetSkyIrradianceLUT(currentFrame);
        CubeMap *skySpecularLUT = skyboxManager->GetSkySpecularLUT(currentFrame);

        imageInfos[GBUFFER_CNT] = {skyIrradianceLUT->sampler, skyIrradianceLUT->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[GBUFFER_CNT], lightingDescriptorSets[currentFrame], GBUFFER_CNT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &(imageInfos[GBUFFER_CNT]));

        imageInfos[GBUFFER_CNT + 1] = {skySpecularLUT->sampler, skySpecularLUT->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[GBUFFER_CNT + 1], lightingDescriptorSets[currentFrame], GBUFFER_CNT + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &(imageInfos[GBUFFER_CNT + 1]));

        imageInfos[GBUFFER_CNT + 2] = {brdfLUT->textureSampler, brdfLUT->textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[GBUFFER_CNT + 2], lightingDescriptorSets[currentFrame], GBUFFER_CNT + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &(imageInfos[GBUFFER_CNT + 2]));

        vkUpdateDescriptorSets(globalLogicalDevice, GBUFFER_CNT + extraTextureCnt, descriptorWrites, 0, nullptr);

        VkWriteDescriptorSet shadowDescriptorWrites[2];
        VkDescriptorImageInfo shadowImageInfo;
        VkDescriptorBufferInfo shadowBufferInfo = shadowPass->GetDirectionalShadowBufferInfo(currentFrame);
        vke_render::ConstructDescriptorSetWrite(shadowDescriptorWrites[0], shadowDescriptorSets[currentFrame], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &shadowBufferInfo);

        shadowImageInfo = {shadowPass->GetShadowMapSampler(), shadowPass->GetDirectionalShadowMapView(currentFrame), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        vke_render::ConstructDescriptorSetWrite(shadowDescriptorWrites[1], shadowDescriptorSets[currentFrame], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &shadowImageInfo);

        vkUpdateDescriptorSets(globalLogicalDevice, 2, shadowDescriptorWrites, 0, nullptr);
    }

    void DeferredLightingPass::createGraphicsPipeline()
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &(context->colorFormat);
        pipelineRenderingCreateInfo.depthAttachmentFormat = context->depthFormat;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;
        pipelineInfo.pDepthStencilState = &depthStencil;

        std::vector<uint32_t> vertexAttributeSizes;
        renderPipeline = std::make_unique<GraphicsPipeline>(lightingShader, vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
    }

    void DeferredLightingPass::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.pNext = nullptr;
        colorAttachmentInfo.imageView = context->colorImageViews[imageIndex];
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.clearValue.color = {{1.0f, 0.5f, 0.3f, 1.0f}};
        colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.pNext = nullptr;
        renderingInfo.renderArea = {{0, 0}, {context->width, context->height}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;
        renderingInfo.pDepthAttachment = nullptr; //&depthAttachmentInfo;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        renderPipeline->Bind(commandBuffer);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(context->width);
        viewport.height = static_cast<float>(context->height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {context->width, context->height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDescriptorSet descriptorSets[3] = {globalDescriptorSets[currentFrame], lightingDescriptorSets[currentFrame], shadowDescriptorSets[currentFrame]};

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderPipeline->pipelineLayout, 0, 3, descriptorSets, 0, nullptr);

        vkCmdPushConstants(commandBuffer, renderPipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0,
                           sizeof(uint32_t), &(lightManager->lightCnts[(int)LightType::DIRECTIONAL_LIGHT]));
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRendering(commandBuffer);
    }

    void DeferredLightingPass::OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx)
    {
    }

    void DeferredLightingPass::onTransientResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame)
    {
        updateDescriptorSet(currentFrame);
    }
}