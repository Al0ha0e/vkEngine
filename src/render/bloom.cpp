#include <render/bloom.hpp>

namespace vke_render
{
    BloomPass::~BloomPass()
    {
        cleanupImageViews();
        cleanupImages();
        if (sampler != VK_NULL_HANDLE)
            vkDestroySampler(globalLogicalDevice, sampler, nullptr);
    }

    void BloomPass::constructFrameGraph(FrameGraph &frameGraph,
                                        std::map<std::string, vke_ds::id32_t> &blackboard,
                                        CurrentResourceNodeIDMaps &currentResourceNodeID)
    {
        vke_ds::id32_t hdrColorResourceID = blackboard.at("hdrColor");
        bloomHdrColorResourceID = frameGraph.AddTransientImageResource("bloomHdrColor", images, VK_IMAGE_ASPECT_COLOR_BIT);
        blackboard["bloomHdrColor"] = bloomHdrColorResourceID;
        vke_ds::id32_t bloomOutColorResourceNodeID = frameGraph.AllocResourceNode("bloomOutHDRColor", true, bloomHdrColorResourceID);

        bloomTaskNodeID = frameGraph.AllocTaskNode("bloom", RENDER_TASK,
                                                   std::bind(&BloomPass::Render, this,
                                                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                                             std::placeholders::_4, std::placeholders::_5));
        frameGraph.SetTaskNodeTransientReadyCallback(bloomTaskNodeID,
                                                     std::bind(&BloomPass::onTransientResourcesReady, this,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        frameGraph.AddTaskNodeResourceRef(bloomTaskNodeID, true, currentResourceNodeID[TRANSIENT_RESOURCE_NODE_MAP][hdrColorResourceID], 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        frameGraph.AddTaskNodeResourceRef(bloomTaskNodeID, true, 0, bloomOutColorResourceNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        currentResourceNodeID[TRANSIENT_RESOURCE_NODE_MAP][hdrColorResourceID] = bloomOutColorResourceNodeID;
    }

    void BloomPass::allocateDescriptorSet()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            bloomDescriptorSets[i] = bloomShader->CreateDescriptorSet(0);
    }

    void BloomPass::createGraphicsPipeline()
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        static constexpr VkFormat hdrFormat = HDRColorManager::HDR_COLOR_FORMAT;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &hdrFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

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
        renderPipeline = std::make_unique<GraphicsPipeline>(bloomShader, vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
    }

    void BloomPass::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.pNext = nullptr;
        colorAttachmentInfo.imageView = imageViews[currentFrame];
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderPipeline->pipelineLayout, 0, 1,
                                &bloomDescriptorSets[currentFrame], 0, nullptr);

        vkCmdPushConstants(commandBuffer, renderPipeline->pipelineLayout, VK_SHADER_STAGE_ALL,
                           0, sizeof(BloomConfig), &constants);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }

    void BloomPass::onTransientResourcesReady(TaskNode &node, FrameGraph &frameGraph, uint32_t currentFrame)
    {
        createImageView(currentFrame);

        VkDescriptorImageInfo hdrColorImageInfo = {
            inputSampler,
            inputImageViewGetter(currentFrame),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        VkWriteDescriptorSet descriptorWrite{};
        ConstructDescriptorSetWrite(descriptorWrite, bloomDescriptorSets[currentFrame], 0,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &hdrColorImageInfo);
        vkUpdateDescriptorSets(globalLogicalDevice, 1, &descriptorWrite, 0, nullptr);
    }

    void BloomPass::createImages()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            RenderEnvironment::CreateImageWithoutMemory(context->width, context->height,
                                                        HDRColorManager::HDR_COLOR_FORMAT, VK_IMAGE_TILING_OPTIMAL,
                                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                            VK_IMAGE_USAGE_SAMPLED_BIT |
                                                            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                        1, &images[i]);
    }

    void BloomPass::cleanupImages()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            if (images[i] != VK_NULL_HANDLE)
            {
                vkDestroyImage(globalLogicalDevice, images[i], nullptr);
                images[i] = VK_NULL_HANDLE;
            }
    }

    void BloomPass::createImageView(uint32_t currentFrame)
    {
        if (imageViews[currentFrame] != VK_NULL_HANDLE)
            vkDestroyImageView(globalLogicalDevice, imageViews[currentFrame], nullptr);
        imageViews[currentFrame] = RenderEnvironment::CreateImageView(images[currentFrame], HDRColorManager::HDR_COLOR_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void BloomPass::cleanupImageViews()
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            if (imageViews[i] != VK_NULL_HANDLE)
            {
                vkDestroyImageView(globalLogicalDevice, imageViews[i], nullptr);
                imageViews[i] = VK_NULL_HANDLE;
            }
    }

    void BloomPass::createSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        VKE_VK_CHECK(vkCreateSampler(globalLogicalDevice, &samplerInfo, nullptr, &sampler), "failed to create bloom sampler!")
    }
}
