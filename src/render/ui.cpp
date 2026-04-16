#include <render/ui.hpp>

namespace vke_render
{
    struct UITextPushConstants
    {
        glm::vec2 viewportSize;
        glm::vec2 atlasSize;
    };

    void UIRenderer::SetText(int x, int y, std::string_view s)
    {
        glyphInstanceCount = 0;

        GlyphInstanceGPU glyphInstances[MAX_UI_CHAR_CNT]{};
        const int originX = x;
        int penX = x;
        int baselineY = y + font->ascender;

        for (char ch : s)
        {
            if (glyphInstanceCount >= MAX_UI_CHAR_CNT)
                break;

            if (ch == '\n')
            {
                penX = originX;
                baselineY += font->lineHeight;
                continue;
            }

            uint32_t codepoint = static_cast<unsigned char>(ch);
            const vke_common::Glyph *glyph = font->GetGlyph(codepoint);
            if (glyph == nullptr)
                glyph = font->GetGlyph('?');
            if (glyph == nullptr)
                continue;

            if (glyph->width > 0 && glyph->height > 0)
            {
                GlyphInstanceGPU &instance = glyphInstances[glyphInstanceCount++];
                const float minX = static_cast<float>(penX + glyph->bearingX);
                const float minY = static_cast<float>(baselineY - glyph->bearingY);
                instance.quadRect = glm::vec4(minX, minY,
                                              minX + static_cast<float>(glyph->width),
                                              minY + static_cast<float>(glyph->height));
                instance.uvRect = glm::vec4(static_cast<float>(glyph->atlasX),
                                            static_cast<float>(glyph->atlasY),
                                            static_cast<float>(glyph->atlasX + glyph->width),
                                            static_cast<float>(glyph->atlasY + glyph->height));
                instance.color = glm::vec4(1.0f);
            }

            penX += glyph->advanceX;
        }

        glyphInstanceBuffer->ToBuffer(0, glyphInstances, sizeof(glyphInstances));
    }

    void UIRenderer::constructFrameGraph(FrameGraph &frameGraph,
                                         std::map<std::string, vke_ds::id32_t> &blackboard,
                                         std::map<vke_ds::id32_t, vke_ds::id32_t> &currentResourceNodeID)
    {
        vke_ds::id32_t colorAttachmentResourceID = blackboard["colorAttachment"];
        vke_ds::id32_t uiOutColorResourceNodeID = frameGraph.AllocResourceNode("uiOutColor", false, colorAttachmentResourceID);

        vke_ds::id32_t glyphInstanceBufferResourceID = frameGraph.AddPermanentBufferResource("uiGlyphInstanceBuffer",
                                                                                             false,
                                                                                             &(glyphInstanceBuffer->buffer), 0, glyphInstanceBuffer->bufferSize,
                                                                                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        vke_ds::id32_t glyphAtlasResourceID = frameGraph.AddPermanentImageResource("uiGlyphAtlas", false, &(font->atlasTexture->textureImage),
                                                                                   VK_IMAGE_ASPECT_COLOR_BIT, false,
                                                                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vke_ds::id32_t glyphBufferResourceNodeID = frameGraph.AllocResourceNode("uiGlyphInstances", false, glyphInstanceBufferResourceID);
        vke_ds::id32_t glyphAtlasResourceNodeID = frameGraph.AllocResourceNode("uiGlyphAtlasNode", false, glyphAtlasResourceID);

        vke_ds::id32_t uiTaskNodeID = frameGraph.AllocTaskNode("ui render", RENDER_TASK,
                                                               std::bind(&UIRenderer::Render, this,
                                                                         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        frameGraph.AddTaskNodeResourceRef(uiTaskNodeID, false, glyphBufferResourceNodeID, 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        frameGraph.AddTaskNodeResourceRef(uiTaskNodeID, false, glyphAtlasResourceNodeID, 0,
                                          VK_ACCESS_SHADER_READ_BIT,
                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        frameGraph.AddTaskNodeResourceRef(uiTaskNodeID, false, currentResourceNodeID[colorAttachmentResourceID], uiOutColorResourceNodeID,
                                          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        currentResourceNodeID[colorAttachmentResourceID] = uiOutColorResourceNodeID;
    }

    void UIRenderer::createDescriptorSet()
    {
        uiDescriptorSet = textShader->CreateDescriptorSet(0);
        VkWriteDescriptorSet descriptorWrites[2];
        VkDescriptorBufferInfo glyphBufferInfo = glyphInstanceBuffer->GetDescriptorBufferInfo();
        VkDescriptorImageInfo atlasImageInfo = {font->atlasTexture->textureSampler, font->atlasTexture->textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[0], uiDescriptorSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &glyphBufferInfo);
        vke_render::ConstructDescriptorSetWrite(descriptorWrites[1], uiDescriptorSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &atlasImageInfo);
        vkUpdateDescriptorSets(globalLogicalDevice, 2, descriptorWrites, 0, nullptr);
    }

    void UIRenderer::createGraphicsPipeline()
    {
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &(context->colorFormat);
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
        renderPipeline = std::make_unique<GraphicsPipeline>(textShader, vertexAttributeSizes, VK_VERTEX_INPUT_RATE_VERTEX, pipelineInfo);
    }

    void UIRenderer::Render(TaskNode &node, FrameGraph &frameGraph, VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex)
    {
        if (glyphInstanceCount == 0)
            return;

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.pNext = nullptr;
        colorAttachmentInfo.imageView = context->colorImageViews[imageIndex];
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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
                                renderPipeline->pipelineLayout, 0, 1, &uiDescriptorSet, 0, nullptr);

        UITextPushConstants pushConstants{
            glm::vec2(static_cast<float>(context->width), static_cast<float>(context->height)),
            glm::vec2(static_cast<float>(font->atlasWidth), static_cast<float>(font->atlasHeight))};
        vkCmdPushConstants(commandBuffer, renderPipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0,
                           sizeof(UITextPushConstants), &pushConstants);

        vkCmdDraw(commandBuffer, 6, glyphInstanceCount, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }

    void UIRenderer::OnWindowResize(FrameGraph &frameGraph, RenderContext *ctx) {}
}