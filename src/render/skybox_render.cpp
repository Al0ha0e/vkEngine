#include <render/render.hpp>
#include <render/skybox_render.hpp>
#include <asset.hpp>

namespace vke_render
{
    void SkyboxRenderer::constructFrameGraph(FrameGraph &frameGraph,
                                             std::map<std::string, vke_ds::id32_t> &blackboard,
                                             std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        vke_ds::id32_t lutTaskID = frameGraph.AllocTaskNode("gen sky lut", COMPUTE_TASK,
                                                            std::bind(&SkyboxRenderer::generateLUT, this,
                                                                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        VkDescriptorImageInfo info{};
        info.imageView = skyLUT->textureImageView;
        vke_ds::id32_t lutResourceID = frameGraph.AddPermanentImageResource("skyLUT", skyLUT->textureImage, VK_IMAGE_ASPECT_COLOR_BIT, info,
                                                                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, std::nullopt);
        vke_ds::id32_t lutOutResourceNodeID = frameGraph.AllocResourceNode("outSkyLUT", false, lutResourceID);

        frameGraph.AddTaskNodeResourceRef(lutTaskID, false, 0, lutOutResourceNodeID,
                                          VK_ACCESS_SHADER_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                          VK_IMAGE_LAYOUT_GENERAL,
                                          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

        vke_ds::id32_t colorAttachmentResourceID = blackboard["colorAttachment"];
        vke_ds::id32_t depthAttachmentResourceID = blackboard["depthAttachment"];
        vke_ds::id32_t cameraResourceID = blackboard["cameraInfo"];

        vke_ds::id32_t skyOutColorResourceNodeID = frameGraph.AllocResourceNode("skyOutColor", false, colorAttachmentResourceID);
        vke_ds::id32_t skyTaskNodeID = frameGraph.AllocTaskNode("skybox render", RENDER_TASK,
                                                                std::bind(&SkyboxRenderer::Render, this,
                                                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        frameGraph.AddTaskNodeResourceRef(skyTaskNodeID, false, currentResourceNodeID[cameraResourceID], 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.AddTaskNodeResourceRef(skyTaskNodeID, false, currentResourceNodeID[colorAttachmentResourceID], skyOutColorResourceNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
        frameGraph.AddTaskNodeResourceRef(skyTaskNodeID, false, currentResourceNodeID[depthAttachmentResourceID], 0,
                                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.AddTaskNodeResourceRef(skyTaskNodeID, false, lutOutResourceNodeID, 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        currentResourceNodeID[colorAttachmentResourceID] = skyOutColorResourceNodeID;
    }

    void SkyboxRenderer::createGraphicsPipeline()
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

    void SkyboxRenderer::initResources()
    {
        auto &paramJSON = vke_common::AssetManager::LoadJSON(std::string(REL_DIR) + "/builtin_assets/config/atmosphere_param.json");
        atmosphereParameter = AtmosphereParameter(paramJSON);
        atmosphereParamBuffer = std::make_unique<DeviceBuffer>(sizeof(AtmosphereParameter), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        atmosphereParamBuffer->ToBuffer(0, &atmosphereParameter, sizeof(AtmosphereParameter));

        std::shared_ptr<ShaderModuleSet> skyLUTShader = vke_common::AssetManager::LoadComputeShaderUnique(vke_common::BUILTIN_COMPUTE_SHADER_SKYLUT_ID);
        skyLUTGenerationPipeline = std::make_unique<ComputePipeline>(std::move(skyLUTShader));

        skyLUT = std::make_unique<Texture2D>(256, 128, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                             VK_IMAGE_LAYOUT_GENERAL,
                                             VK_FILTER_LINEAR,
                                             VK_FILTER_LINEAR,
                                             VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                             false);
        skyboxMaterial = vke_common::AssetManager::LoadMaterialUnique(vke_common::BUILTIN_MATERIAL_SKYBOX_ID);
        // skybox mesh
        std::vector<uint32_t> indices;
        for (int i = 0; i < skyboxVertices.size(); i++)
            indices.push_back(i);
        skyboxMesh = std::make_unique<Mesh>(0, skyboxVertices.size() * sizeof(glm::vec3), (void *)skyboxVertices.data(), indices);
    }

    void SkyboxRenderer::createDescriptorSet()
    {
        skyBoxDescriptorSet = skyboxMaterial->shader->CreateDescriptorSet(1);
        skyboxMaterial->UpdateDescriptorSet(skyBoxDescriptorSet, 1);

        std::vector<VkWriteDescriptorSet> descriptorWrites(3);
        // layout (std430, set = 1, binding = 0) uniform UBO { AtmosphereParameter parameters; };
        VkDescriptorBufferInfo bufferInfo{};
        vke_render::InitDescriptorBufferInfo(bufferInfo, atmosphereParamBuffer->buffer, 0, atmosphereParamBuffer->bufferSize);
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[0], skyBoxDescriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        // layout(set = 1, binding = 3) uniform sampler2D skyViewLUT;
        VkDescriptorImageInfo imageInfo1{skyLUT->textureSampler, skyLUT->textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[1], skyBoxDescriptorSet, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo1);
        // layout(set = 1, binding = 4, rgba8) uniform writeonly image2D skyViewImage;
        VkDescriptorImageInfo imageInfo2{nullptr, skyLUT->textureImageView, VK_IMAGE_LAYOUT_GENERAL};
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[2], skyBoxDescriptorSet, 4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo2);
        vkUpdateDescriptorSets(RenderEnvironment::GetInstance()->logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    void SkyboxRenderer::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
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
        glm::vec4 sunLightDir = glm::normalize(glm::vec4(1, 0.08, 0, 0));
        vkCmdPushConstants(commandBuffer, renderPipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(glm::vec4), &sunLightDir);
        skyboxMesh->Render(commandBuffer);

        vkCmdEndRendering(commandBuffer);
    }

    void SkyboxRenderer::generateLUT(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        glm::vec4 sunLightDir = glm::normalize(glm::vec4(1, 0.08, 0, 0));
        skyLUTGenerationPipeline->Dispatch(commandBuffer, std::vector<VkDescriptorSet>{globalDescriptorSet, skyBoxDescriptorSet},
                                           &sunLightDir, glm::ivec3(8, 4, 1));
    }
}