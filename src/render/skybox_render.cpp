#include <render/render.hpp>
#include <render/skybox_render.hpp>
#include <asset.hpp>

namespace vke_render
{
    void BaseRenderer::constructFrameGraph(FrameGraph &frameGraph,
                                           std::map<std::string, vke_ds::id32_t> &blackboard,
                                           std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        vke_ds::id32_t colorAttachmentResourceID = blackboard["colorAttachment"];
        vke_ds::id32_t depthAttachmentResourceID = blackboard["depthAttachment"];
        vke_ds::id32_t cameraResourceID = blackboard["cameraInfo"];

        vke_ds::id32_t baseOutColorResourceNodeID = frameGraph.AllocResourceNode("baseOutColor", false, colorAttachmentResourceID);
        vke_ds::id32_t baseTaskNodeID = frameGraph.AllocTaskNode("base render", RENDER_TASK,
                                                                 std::bind(&BaseRenderer::Render, this,
                                                                           std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        frameGraph.AddTaskNodeResourceRef(baseTaskNodeID, false, currentResourceNodeID[cameraResourceID], 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.AddTaskNodeResourceRef(baseTaskNodeID, false, currentResourceNodeID[colorAttachmentResourceID], baseOutColorResourceNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
        frameGraph.AddTaskNodeResourceRef(baseTaskNodeID, false, currentResourceNodeID[depthAttachmentResourceID], 0,
                                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        currentResourceNodeID[colorAttachmentResourceID] = baseOutColorResourceNodeID;
    }

    void BaseRenderer::createGraphicsPipeline()
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &(context->colorFormat);
        pipelineRenderingCreateInfo.depthAttachmentFormat = context->depthFormat;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {};  // Optional

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;
        pipelineInfo.pDepthStencilState = &depthStencil;

        std::vector<uint32_t> vertexAttributeSizes = {sizeof(glm::vec3)};
        renderPipeline = std::make_unique<GraphicsPipeline>(skyboxMaterial->shader, vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
    }

    const std::vector<glm::vec3> skyboxVertices = {
        // positions
        {-1.0f, 1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},
        {-1.0f, 1.0f, -1.0f},

        {-1.0f, -1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f, 1.0f, -1.0f},
        {-1.0f, 1.0f, -1.0f},
        {-1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f},

        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},

        {-1.0f, -1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f},

        {-1.0f, 1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, -1.0f},

        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f}};

    void BaseRenderer::createSkyBox()
    {
        skyboxMaterial = vke_common::AssetManager::LoadMaterialUnique(vke_common::BUILTIN_MATERIAL_SKYBOX_ID);
        skyBoxDescriptorSet = skyboxMaterial->shader->CreateDescriptorSet(1);
        skyboxMaterial->UpdateDescriptorSet(skyBoxDescriptorSet);
        // skybox mesh
        std::vector<uint32_t> indices;
        for (int i = 0; i < skyboxVertices.size(); i++)
            indices.push_back(i);
        skyboxMesh = std::make_unique<Mesh>(0, skyboxVertices.size() * sizeof(glm::vec3), (void *)skyboxVertices.data(), indices);
    }

    void BaseRenderer::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.pNext = nullptr;
        colorAttachmentInfo.imageView = (*context->colorImageViews)[imageIndex];
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;

        VkRenderingAttachmentInfo depthAttachmentInfo{};
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachmentInfo.pNext = nullptr;
        depthAttachmentInfo.imageView = context->depthImageView;
        depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.pNext = nullptr;
        renderingInfo.renderArea = {{0, 0}, {context->width, context->height}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;

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

        VkDescriptorSet descriptorSets[2] = {globalDescriptorSet, skyBoxDescriptorSet};

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderPipeline->pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
        skyboxMesh->Render(commandBuffer);

        vkCmdEndRendering(commandBuffer);
    }
}