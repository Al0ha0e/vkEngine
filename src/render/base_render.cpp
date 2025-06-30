#include <render/render.hpp>
#include <render/base_render.hpp>
#include <asset.hpp>

namespace vke_render
{
    void BaseRenderer::constructFrameGraph(FrameGraph &frameGraph,
                                           std::map<std::string, vke_ds::id32_t> &blackboard,
                                           std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        vke_ds::id32_t colorAttachmentResourceID = blackboard["colorAttachment"];
        vke_ds::id32_t cameraResourceID = blackboard["cameraInfo"];

        vke_ds::id32_t baseOutColorResourceNodeID = frameGraph.AllocResourceNode(false, colorAttachmentResourceID);
        vke_ds::id32_t baseTaskNodeID = frameGraph.AllocTaskNode(RENDER_TASK,
                                                                 std::bind(&BaseRenderer::Render, this,
                                                                           std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        frameGraph.AddTaskNodeResourceRef(baseTaskNodeID, false, currentResourceNodeID[cameraResourceID], 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.AddTaskNodeResourceRef(baseTaskNodeID, false, currentResourceNodeID[colorAttachmentResourceID], baseOutColorResourceNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

        currentResourceNodeID[colorAttachmentResourceID] = baseOutColorResourceNodeID;
    }

    void BaseRenderer::createGlobalDescriptorSet()
    {
        VkDescriptorSetLayoutBinding vpLayoutBinding{};
        vpLayoutBinding.binding = 0;
        vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vpLayoutBinding.descriptorCount = 1;
        vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vpLayoutBinding.pImmutableSamplers = nullptr; // Optional

        globalDescriptorInfos.push_back(DescriptorInfo(vpLayoutBinding, sizeof(CameraInfo)));

        globalDescriptorSetInfo.uniformDescriptorCnt = 0;
        std::vector<VkDescriptorSetLayoutBinding> globalBindings;
        for (auto &dInfo : globalDescriptorInfos)
        {
            globalDescriptorSetInfo.AddCnt(dInfo.bindingInfo);
            globalBindings.push_back(dInfo.bindingInfo);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = globalBindings.size();
        layoutInfo.pBindings = globalBindings.data();

        if (vkCreateDescriptorSetLayout(environment->logicalDevice,
                                        &layoutInfo,
                                        nullptr,
                                        &(globalDescriptorSetInfo.layout)) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        globalDescriptorSet = vke_render::DescriptorSetAllocator::AllocateDescriptorSet(globalDescriptorSetInfo);
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
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
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
        renderInfo->CreatePipeline(vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
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
        std::shared_ptr<Material> skyboxMaterial(new Material);

        std::shared_ptr<Texture2D> texture = vke_common::AssetManager::LoadTexture2D(vke_common::BUILTIN_TEXTURE_SKYBOX_ID);
        skyboxMaterial->textures.push_back(texture);
        skyboxMaterial->shader = vke_common::AssetManager::LoadVertFragShader(vke_common::BUILTIN_VFSHADER_SKYBOX_ID);

        VkDescriptorSetLayoutBinding textureLayoutBinding{};
        textureLayoutBinding.binding = 0;
        textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureLayoutBinding.descriptorCount = 1;
        textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        textureLayoutBinding.pImmutableSamplers = nullptr; // Optional
        vke_render::DescriptorInfo textureDescriptorInfo(textureLayoutBinding, texture->textureImageView, texture->textureSampler);
        skyboxMaterial->commonDescriptorInfos.push_back(std::move(textureDescriptorInfo));

        renderInfo = std::make_unique<RenderInfo>(skyboxMaterial);

        // skybox mesh
        std::vector<uint32_t> indices;
        for (int i = 0; i < skyboxVertices.size(); i++)
            indices.push_back(i);
        std::shared_ptr<Mesh> skyboxMesh = std::make_shared<Mesh>(0, skyboxVertices.size() * sizeof(glm::vec3), (void *)skyboxVertices.data(), indices);

        std::vector<std::unique_ptr<vke_render::HostCoherentBuffer>> buffers;
        renderInfo->AddUnit(skyboxMesh, buffers);
        createGraphicsPipeline();
    }

    void BaseRenderer::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.pNext = nullptr;
        colorAttachmentInfo.imageView = (*context->colorImageViews)[imageIndex];
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
        renderingInfo.pDepthAttachment = nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo->pipeline);
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

        renderInfo->Render(commandBuffer, &globalDescriptorSet);

        vkCmdEndRendering(commandBuffer);
    }
}