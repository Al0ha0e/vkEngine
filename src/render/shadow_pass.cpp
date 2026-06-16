#include <render/shadow_pass.hpp>

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

        constructFrameGraph(frameGraph, blackboard, currentResourceNodeID);
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
        const DirectionalShadowConfig &config = shadowManager->GetConfig();
        shadowMapResourceID = frameGraph.AddTransientImageResource("directionalShadowMap0", shadowManager->GetDirectionalShadowMapImages(), VK_IMAGE_ASPECT_DEPTH_BIT, 1, config.cascadeCnt);
        vke_ds::id32_t shadowMapOutResourceNodeID = frameGraph.AllocResourceNode("directionalShadowMap0Out", shadowMapResourceID);

        blackboard["directionalShadowMap0"] = shadowMapResourceID;
        blackboard["directionalShadowMap0OutNode"] = shadowMapOutResourceNodeID;

        shadowTaskNodeID = frameGraph.AllocTaskNode("shadow pass", RENDER_TASK,
                                                    std::bind(&ShadowPass::Render, this,
                                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        frameGraph.AddTaskNodeResourceRef(shadowTaskNodeID, 0, shadowMapOutResourceNodeID,
                                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        currentResourceNodeID[shadowMapResourceID] = shadowMapOutResourceNodeID;
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

    void ShadowPass::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        const DirectionalShadowConfig &config = shadowManager->GetConfig();
        const DirectionalShadowInfoCPU &directionalShadowInfo = shadowManager->GetDirectionalShadowInfo();
        if (directionalShadowInfo.lightIndex.x == INVALID_SHADOW_LIGHT_INDEX)
            return;

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
            depthAttachmentInfo.imageView = shadowManager->GetDirectionalShadowCascadeView(currentFrame, cascade);
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
                renderInfo->Render(commandBuffer, shadowManager->GetShadowPassDescriptorSet(currentFrame));
            }

            vkCmdEndRendering(commandBuffer);
        }
    }

    void ShadowPass::OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx)
    {
    }
}
